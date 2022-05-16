namespace boost
{
namespace filesystem
{
	class path;
}
}

namespace vxldollar
{
class node_flags;
}
namespace vxldollar_daemon
{
class daemon
{
public:
	void run (boost::filesystem::path const &, vxldollar::node_flags const & flags);
};
}
