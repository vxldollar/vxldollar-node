#include <vxldollar/lib/utility.hpp>
#include <vxldollar/secure/working.hpp>

#include <pwd.h>
#include <sys/types.h>

namespace vxldollar
{
boost::filesystem::path app_path ()
{
	auto entry (getpwuid (getuid ()));
	debug_assert (entry != nullptr);
	boost::filesystem::path result (entry->pw_dir);
	return result;
}
}
