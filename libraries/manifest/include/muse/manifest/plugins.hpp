
#pragma once

#include <memory>
#include <string>
#include <vector>

namespace muse { namespace app {

class abstract_plugin;

} }

namespace muse { namespace plugin {

void initialize_plugin_factories();
std::shared_ptr< muse::app::abstract_plugin > create_plugin( const std::string& name );
std::vector< std::string > get_available_plugins();

} }
