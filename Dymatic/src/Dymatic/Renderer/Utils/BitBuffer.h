#pragma once

#include <vector>

namespace Dymatic {

	class BitBuffer
	{
	protected:
		BitBuffer() = default;
	protected:
		std::vector<uint8_t> m_Buffer;
	};

	class BitBufferWriter : public BitBuffer
	{
	public:
		BitBufferWriter() = default;

		// Insertion

		void AddBit(bool bit)
		{
			if (bit)
				m_CurrentByte |= (1 << m_CurrentBitIndex);

			m_CurrentBitIndex++;

			// If we have a complete byte, push it to the buffer and reset
			if (m_CurrentBitIndex == 8)
			{
				m_Buffer.push_back(m_CurrentByte);
				m_CurrentByte = 0;
				m_CurrentBitIndex = 0;
			}
		}

		void AddBits(const std::vector<bool>& bits)
		{
			for (const bool bit : bits)
				AddBit(bit);
		}

		void AddByte(uint8_t byte)
		{
			for (int i = 0; i < 8; i++)
				AddBit((byte >> i) & 1);
		}

		inline const std::vector<uint8_t>& GetBuffer() const { return m_Buffer; }
		inline const std::vector<uint8_t>& GetFinalBuffer()
		{
			if (m_CurrentBitIndex > 0)
				m_Buffer.push_back(m_CurrentByte);

			return GetBuffer();
		}

	private:
		uint8_t m_CurrentByte = 0;
		uint8_t m_CurrentBitIndex = 0;
	};

	class BitBufferReader : public BitBuffer
	{
	public:
		BitBufferReader(const uint8_t* data, size_t size)
		{
			m_Buffer.assign(data, data + size);
		}

		bool HasBits() const
		{
			return m_CurrentBitIndex < (m_Buffer.size() * 8);
		}

		bool GetNextBit()
		{
			if (!HasBits())
			{
				// This is invalid behavior, not a '0' bit
				DY_CORE_ASSERT(false, "No bits remaining in bit buffer");
				return false;
			}

			const size_t byteIndex = m_CurrentBitIndex / 8;
			const size_t bitIndex = m_CurrentBitIndex % 8;
			m_CurrentBitIndex++;

			return (m_Buffer[byteIndex] >> bitIndex) & 1;
		}

		uint8_t GetNextByte()
		{
			if (!HasBits())
			{
				// This is invalid behavior, not a '0' byte
				DY_CORE_ASSERT(false, "No bits remaining in bit buffer");
				return 0;
			}

			uint8_t byte = 0;

			for (int i = 0; i < 8; i++)
			{
				if (!HasBits())
					continue;

				size_t byteIndex = m_CurrentBitIndex / 8;
				size_t bitIndex = m_CurrentBitIndex % 8;

				// Extract the current bit and place it in the correct position in the byte
				if (m_Buffer[byteIndex] & (1 << bitIndex))
					byte |= (1 << i);

				m_CurrentBitIndex++;
			}

			return byte;
		}

	private:
		size_t m_CurrentBitIndex = 0;
	};

}