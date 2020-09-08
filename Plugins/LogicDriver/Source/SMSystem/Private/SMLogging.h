// Copyright Recursoft LLC 2019-2020. All Rights Reserved.
#pragma once

#include "ISMSystemModule.h"
#include "Stats/Stats.h"

#define LOG_INFO(FMT, ...) UE_LOG(LogLogicDriver, Info, (FMT), ##__VA_ARGS__)
#define LOG_WARNING(FMT, ...) UE_LOG(LogLogicDriver, Warning, (FMT), ##__VA_ARGS__)
#define LOG_ERROR(FMT, ...) UE_LOG(LogLogicDriver, Error, (FMT), ##__VA_ARGS__)

DECLARE_STATS_GROUP(TEXT("SMSystem"), STATGROUP_SMSystem, STATCAT_Advanced)