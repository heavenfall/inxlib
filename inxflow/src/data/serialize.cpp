#include <inxflow/data/serialize.hpp>

namespace inx::flow::data
{

void SerializeWrapper::load(std::istream& in, const std::filesystem::path& fname, StreamType type)
{
	wrapper_input send{wrapper_op::Load, type, &m_data, &in, &fname};
	(*m_operators)(send);
}

void SerializeWrapper::save(std::ostream& out, const std::filesystem::path& fname, StreamType type)
{
	wrapper_input send{wrapper_op::Save, type, &m_data, &out, &fname};
	(*m_operators)(send);
}

}
