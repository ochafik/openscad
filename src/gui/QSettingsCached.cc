#include <memory>
#include "gui/QSettingsCached.h"

std::unique_ptr<QSettings> QSettingsCached::qsettingsPointer;
std::mutex QSettingsCached::ctor_mutex;
