#pragma once

#include <cassert>
#include <cstdint>
#include <limits>

#include "Skirnir/Hotswap/module/Serializer.hpp"

namespace skr::hotswap
{
    enum class SwapPhase
    {
        BeforeSwap,
        AfterSwap,
    };

    class SwapInfo
    {
      public:
        [[nodiscard]] SwapPhase Phase() const
        {
            return m_Phase;
        }

        [[nodiscard]] std::uint64_t Id() const
        {
            return m_Id;
        }

        void SetInitCb(const std::function<void()>& cb)
        {
            m_InitCb = cb;
        }

        template <typename T>
        void Save(const std::string& name, T& val)
        {
            switch (m_Phase)
            {
                case SwapPhase::BeforeSwap:
                    Serialize(name, val);
                    break;
                case SwapPhase::AfterSwap:
                    Unserialize(name, val);
                    break;
                default:
                    throw std::runtime_error("failed to save swap info.");
                    break;
            }
        }

        template <typename T>
        void Serialize(const std::string& name, const T& val)
        {
            m_Serializer.SerializeCopy(name, val);
        }

        template <typename T>
        bool Unserialize(const std::string& name, T& val)
        {
            return m_Serializer.UnserializeCopy(name, val);
        }

        template <typename T>
        void SaveMove(const std::string& name, T&& val)
        {
            switch (m_Phase)
            {
                case SwapPhase::BeforeSwap:
                    SerializeMove(name, std::move(val));
                    break;
                case SwapPhase::AfterSwap:
                    Unserialize(name, val);
                    break;
                default:
                    assert(false);
                    break;
            }
        }

        template <typename T>
        void SerializeMove(const std::string& name, T&& val)
        {
            m_Serializer.SerializeMove(name, std::move(val));
        }

        template <typename T>
        bool UnserializeMove(const std::string& name, T& val)
        {
            return m_Serializer.UnserializeMove(name, val);
        }

      private:
        friend class ModuleInterface;

        SwapPhase m_Phase = {};
        std::uint64_t m_Id = (std::numeric_limits<std::uint64_t>::max)();
        Serializer m_Serializer;
        std::function<void()> m_InitCb;

        void TriggerInitCb()
        {
            if (m_InitCb != nullptr)
            {
                m_InitCb();
            }
        }
    };
} // namespace skr::hotswap
