#include <iostream>

// This is the first gcc header to be included
#include "gcc-plugin.h"
#include "plugin-version.h"

// We must assert that this plugin is GPL compatible
int plugin_is_GPL_compatible;

int plugin_init (struct plugin_name_args *plugin_info,
	     struct plugin_gcc_version *version)
{
  // We check the current gcc loading this plugin against the gcc we used to
  // created this plugin
  if (!plugin_default_version_check (version, &gcc_version))
    {
      std::cerr << "This GCC plugin is for version " << GCCPLUGIN_VERSION_MAJOR
	<< "." << GCCPLUGIN_VERSION_MINOR << "\n";
      return 1;
    }

  // Let's print all the information given to this plugin!

  std::cerr << "Plugin info\n";
  std::cerr << "===========\n\n";
  std::cerr << "Base name: " << plugin_info->base_name << "\n";
  std::cerr << "Full name: " << plugin_info->full_name << "\n";
  std::cerr << "Number of arguments of this plugin:" << plugin_info->
    argc << "\n";

  for (int i = 0; i < plugin_info->argc; i++)
    {
      std::cerr << "Argument " << i << ": Key: " << plugin_info->argv[i].
	key << ". Value: " << plugin_info->argv[i].value << "\n";

    }
  if (plugin_info->version != NULL)
    std::cerr << "Version string of the plugin: " << plugin_info->
      version << "\n";
  if (plugin_info->help != NULL)
    std::cerr << "Help string of the plugin: " << plugin_info->help << "\n";

  std::cerr << "\n";
  std::cerr << "Version info\n";
  std::cerr << "============\n\n";
  std::cerr << "Base version: " << version->basever << "\n";
  std::cerr << "Date stamp: " << version->datestamp << "\n";
  std::cerr << "Dev phase: " << version->devphase << "\n";
  std::cerr << "Revision: " << version->devphase << "\n";
  std::cerr << "Configuration arguments: " << version->
    configuration_arguments << "\n";
  std::cerr << "\n";

  std::cerr << "Plugin successfully initialized\n";

  return 0;
}
