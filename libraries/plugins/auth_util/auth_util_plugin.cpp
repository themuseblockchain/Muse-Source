

#include <muse/plugins/auth_util/auth_util_api.hpp>
#include <muse/plugins/auth_util/auth_util_plugin.hpp>

#include <string>

namespace muse { namespace plugin { namespace auth_util {

auth_util_plugin::auth_util_plugin() {}
auth_util_plugin::~auth_util_plugin() {}

std::string auth_util_plugin::plugin_name()const
{
   return "auth_util";
}

void auth_util_plugin::plugin_initialize( const boost::program_options::variables_map& options )
{
}

void auth_util_plugin::plugin_startup()
{
   app().register_api_factory< auth_util_api >( "auth_util_api" );
}

void auth_util_plugin::plugin_shutdown()
{
}

} } } // muse::plugin::auth_util

MUSE_DEFINE_PLUGIN( auth_util, muse::plugin::auth_util::auth_util_plugin )
