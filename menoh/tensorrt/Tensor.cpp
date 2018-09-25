//
// Copyright © 2017 Arm Ltd. All rights reserved.
// See LICENSE file in the project root for full license information.
//

#include <menoh/tensorrt/TypesUtils.hpp>
#include <menoh/tensorrt/Tensor.hpp>
#include <menoh/tensorrt/Exception.hpp>

#include <boost/assert.hpp>
#include <boost/log/trivial.hpp>
#include <boost/numeric/conversion/cast.hpp>

namespace menoh_impl {
namespace tensorrt_backend {

// ---
// --- TensorShape
// ---

TensorShape::TensorShape()
 : m_NumDimensions(0)
{
}

TensorShape::TensorShape(const unsigned int numDimensions, const unsigned int* const dimensionSizes)
 : m_NumDimensions(numDimensions)
{
    if (numDimensions < 1)
    {
        throw InvalidArgumentException("Tensor numDimensions must be greater than 0");
    }

    if (numDimensions > MaxNumOfTensorDimensions)
    {
        throw InvalidArgumentException("Tensor numDimensions must be less than or equal to MaxNumOfTensorDimensions");
    }

    if (dimensionSizes == nullptr)
    {
        throw InvalidArgumentException("Tensor dimensionSizes must not be NULL");
    }

    std::copy(dimensionSizes, dimensionSizes + numDimensions, m_Dimensions.begin());
}

TensorShape::TensorShape(std::initializer_list<unsigned int> dimensionSizeList)
 : TensorShape(boost::numeric_cast<unsigned int>(dimensionSizeList.size()), dimensionSizeList.begin())
{
}

TensorShape::TensorShape(const TensorShape& other)
 : m_NumDimensions(other.m_NumDimensions)
{
    std::copy(other.m_Dimensions.cbegin(), other.m_Dimensions.cbegin() + other.m_NumDimensions, m_Dimensions.begin());
}

TensorShape& TensorShape::operator =(const TensorShape& other)
{
    m_NumDimensions = other.m_NumDimensions;
    std::copy(other.m_Dimensions.cbegin(), other.m_Dimensions.cbegin() + other.m_NumDimensions, m_Dimensions.begin());
    return *this;
}

bool TensorShape::operator==(const TensorShape& other) const
{
    return ((m_NumDimensions == other.m_NumDimensions) &&
        std::equal(m_Dimensions.cbegin(), m_Dimensions.cbegin() + m_NumDimensions, other.m_Dimensions.cbegin()));
}

bool TensorShape::operator!=(const TensorShape& other) const
{
    return !(*this == other);
}

unsigned int TensorShape::GetNumElements() const
{
    if (m_NumDimensions == 0)
    {
        return 0;
    }

    unsigned int count = 1;
    for (unsigned int i = 0; i < m_NumDimensions; i++)
    {
        count *= m_Dimensions[i];
    }

    return count;
}

// ---
// --- TensorInfo
// ---

TensorInfo::TensorInfo()
: m_DataType(DataType::kFLOAT)
{
}

TensorInfo::TensorInfo(const TensorShape& shape, DataType dataType,
    float quantizationScale, int32_t quantizationOffset)
 : m_Shape(shape)
 , m_DataType(dataType)
{
    m_Quantization.m_Scale = quantizationScale;
    m_Quantization.m_Offset = quantizationOffset;
}

TensorInfo::TensorInfo(unsigned int numDimensions, const unsigned int* dimensionSizes, DataType dataType,
    float quantizationScale, int32_t quantizationOffset)
 : m_Shape(numDimensions, dimensionSizes)
 , m_DataType(dataType)
{
    m_Quantization.m_Scale = quantizationScale;
    m_Quantization.m_Offset = quantizationOffset;
}

TensorInfo::TensorInfo(const TensorInfo& other)
: m_Shape(other.m_Shape)
, m_DataType(other.m_DataType)
, m_Quantization(other.m_Quantization)
{
}

TensorInfo& TensorInfo::operator=(const TensorInfo& other)
{
    m_Shape = other.m_Shape;
    m_DataType = other.m_DataType;
    m_Quantization = other.m_Quantization;
    return *this;
}

bool TensorInfo::operator==(const TensorInfo& other) const
{
    return ((m_Shape == other.m_Shape) &&
            (m_DataType == other.m_DataType) &&
            (m_Quantization == other.m_Quantization));
}

bool TensorInfo::operator!=(const TensorInfo& other) const
{
    return !(*this == other);
}

unsigned int TensorInfo::GetNumBytes() const
{
    return GetDataTypeSize(m_DataType) * GetNumElements();
}

// ---
// --- BaseTensor
// ---

template<typename MemoryType>
BaseTensor<MemoryType>::BaseTensor()
 : m_MemoryArea(nullptr)
{
}

template<typename MemoryType>
BaseTensor<MemoryType>::BaseTensor(const TensorInfo& info, MemoryType memoryArea)
 : m_MemoryArea(memoryArea)
 , m_Info(info)
{
}

template<typename MemoryType>
BaseTensor<MemoryType>::BaseTensor(const BaseTensor<MemoryType>& other)
 : m_MemoryArea(other.m_MemoryArea)
 , m_Info(other.GetInfo())
{
}

template<typename MemoryType>
BaseTensor<MemoryType>& BaseTensor<MemoryType>::operator =(const BaseTensor<MemoryType>& other)
{
    m_Info = other.m_Info;
    m_MemoryArea = other.m_MemoryArea;
    return *this;
}

// Explicit instantiations.
template class BaseTensor<const void*>;
template class BaseTensor<void*>;

} // namespace tensorrt_backend
} // namespace menoh_impl

