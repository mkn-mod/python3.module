

#include "mkn/kul/signal.hpp"
#include "mkn/kul/yaml.hpp"
#include <maiken.hpp>

const std::string yArgs = "";

int main(int argc, char *argv[]) {
  mkn::kul::Signal sig;
  try {
    YAML::Node node = mkn::kul::yaml::String(yArgs).root();
    char *argv2[2] = {argv[0], (char *)"-O"};
    auto app = (maiken::Application::CREATE(2, argv2))[0];
    auto loader(maiken::ModuleLoader::LOAD(*app));
    loader->module()->init(*app, node);
    loader->module()->compile(*app, node);
    loader->module()->link(*app, node);
    loader->module()->pack(*app, node);
    loader->unload();
    for (const auto inc : app->includes())
      KLOG(INF) << inc.first;
  } catch (const mkn::kul::Exception &e) {
    KLOG(ERR) << e.what();
    return 2;
  } catch (const std::exception &e) {
    KERR << e.what();
    return 3;
  } catch (...) {
    KERR << "UNKNOWN EXCEPTION TYPE CAUGHT";
    return 5;
  }
  return 0;
}
