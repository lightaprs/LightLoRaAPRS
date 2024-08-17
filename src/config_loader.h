#ifndef config_loader_H
#define config_loader_H

#include "configuration.h"

extern ConfigurationCommon commonConfig;
extern ConfigurationTracker trackerConfig;
extern ConfigurationGateway gatewayConfig;
extern ConfigurationRouter routerConfig;
extern ConfigurationMessaging messagingConfig;

void load_config();
bool isInvalidConfig();

#endif