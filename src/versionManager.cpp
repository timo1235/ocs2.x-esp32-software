#include <includes.h>

VERSIONMANAGER versionManager;

void VERSIONMANAGER::setup()
{
    this->versionInfo = mainConfig.versionInfo;
}

void VERSIONMANAGER::setVersionInfo(VERSION_INFO versionInfo)
{
    this->versionInfo = versionInfo;
}

VERSION_INFO VERSIONMANAGER::getVersionInfo()
{
    return this->versionInfo;
}

bool VERSIONMANAGER::isAtLeast(int major, int minor)
{
    return this->versionInfo.major > major || (this->versionInfo.major == major && this->versionInfo.minor >= minor);
}

bool VERSIONMANAGER::isHigherThan(int major, int minor)
{
    return this->versionInfo.major > major || (this->versionInfo.major == major && this->versionInfo.minor > minor);
}

bool VERSIONMANAGER::isLowerThan(int major, int minor)
{
    return this->versionInfo.major < major || (this->versionInfo.major == major && this->versionInfo.minor < minor);
}

bool VERSIONMANAGER::isBoardType(BOARD_TYPE type)
{
    return this->versionInfo.boardType == type;
}

bool VERSIONMANAGER::isVersion(int major, int minor)
{
    return this->versionInfo.major == major && this->versionInfo.minor == minor;
}