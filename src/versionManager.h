#pragma once

#include <Arduino.h>
#include <map>

enum BOARD_TYPE { OCS2, OCS2_Mini, undefined };

struct VERSION_INFO {
    BOARD_TYPE boardType;
    int major;
    int minor;
};

class VERSIONMANAGER {

  public:
    void setup();
    VERSION_INFO getVersionInfo();
    void setVersionInfo(VERSION_INFO versionInfo);

    bool isAtLeast(int major, int minor);
    bool isHigherThan(int major, int minor);
    bool isLowerThan(int major, int minor);
    bool isVersion(int major, int minor);
    bool isBoardType(BOARD_TYPE type);

  private:
    VERSION_INFO versionInfo = {BOARD_TYPE::undefined, 0, 0};
};
