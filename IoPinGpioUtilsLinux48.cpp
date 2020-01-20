/*
 * IoPinGpioUtilsLinux48.cpp
 *
 *  Created on: 17.01.2020
 *      Author: lode
 */

#include "IoPinGpioUtilsLinux48.h"
#include "gpio-utils.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <dirent.h>
#include <cassert>

namespace Hardware
{

::std::string IoPinGpioUtilsLinux48::globalConsumer;
::std::string IoPinGpioUtilsLinux48::basePath("/dev/");

IoPinGpioUtilsLinux48::IoPinGpioUtilsLinux48(const char* consumer, const char* pinName, const char* gpioChip)
: consumer(consumer ? consumer : not globalConsumer.empty() ? globalConsumer : ::std::to_string(reinterpret_cast<uintptr_t>(this))),
  handleFd(-1),
  cfgInput(true),
  cfgActiveHigh(true),
  cfgOpenDrain(false),
  cfgOpenSource(false),
  isRequested(false),
  outputIsActive(false)
{
  if (gpioChip)
    inPort(gpioChip);

  if (pinName)
    findPin(pinName);
}

IoPinGpioUtilsLinux48::IoPinGpioUtilsLinux48(const char* consumer, const char* gpioChip, int pinOffset)
: lineIndex(~0),
  handleFd(-1),
  myErrno(0),
  cfgInput(true),
  cfgActiveHigh(true),
  cfgOpenDrain(false),
  cfgOpenSource(false),
  isRequested(false),
  outputIsActive(false)
{
  assert(gpioChip);
  inPort(gpioChip).atPin(pinOffset);
}

IoPinGpioUtilsLinux48::~IoPinGpioUtilsLinux48()
{
  if (handleFd > -1)
    gpiotools_release_linehandle(handleFd);
}

void IoPinGpioUtilsLinux48::setGlobalConsumer(const char* consumer)
{
  globalConsumer = consumer;
}

void IoPinGpioUtilsLinux48::setConsumer(const ::std::string& consumer)
{
  if (not consumer.empty())
    this->consumer = consumer;
  else
    this->consumer = globalConsumer;

  if (isRequested)
    requestLine(true);
}

IoPinGpioUtilsLinux48& IoPinGpioUtilsLinux48::inPort(::std::string const& device)
{
  chipName = device;
  return *this;
}

IoPinGpioUtilsLinux48& IoPinGpioUtilsLinux48::atPin(unsigned int offset)
{
  lineIndex = offset;
  return *this;
}

bool IoPinGpioUtilsLinux48::locateNameInChip(::std::string const& chipName, ::std::string const& name)
{
  bool success = false;

  do
  {
    struct gpiochip_info cinfo;
    ::std::string const chipPath(basePath + "/" + chipName);
    int fd;
    int ret;
    unsigned i;

    fd = open(chipPath.c_str(), 0);
    if (fd == -1) {
      myErrno = errno;
      fprintf(stderr, "Failed to open %s\n", chipPath.c_str());
      break;
    }

    /* Inspect this GPIO chip */
    ret = ioctl(fd, GPIO_GET_CHIPINFO_IOCTL, &cinfo);
    if (ret == -1)
    {
      myErrno = errno;
      perror("Failed to read CHIPINFO using IOCTL\n");
      break;
    }

//    fprintf(stdout, "GPIO chip: %s, \"%s\", %u GPIO lines\n",
//            cinfo.name, cinfo.label, cinfo.lines);

    /* Loop over the lines and check info */
    for (i = 0; i < cinfo.lines; i++)
    {
      struct gpioline_info linfo;

      memset(&linfo, 0, sizeof(linfo));
      linfo.line_offset = i;

      ret = ioctl(fd, GPIO_GET_LINEINFO_IOCTL, &linfo);
      if (ret == -1) {
        myErrno = errno;
        perror("Failed to issue LINEINFO IOCTL\n");
        break;
      }

      if ((name == linfo.name) and not linfo.consumer[0] and not (linfo.flags & GPIOLINE_FLAG_KERNEL))
      {
        this->chipName = chipName;
        this->lineName = name;
        this->lineIndex = i;

        cfgInput = !(linfo.flags & GPIOLINE_FLAG_IS_OUT);
        cfgActiveHigh = !(linfo.flags & GPIOLINE_FLAG_ACTIVE_LOW);
        cfgOpenDrain = !!(linfo.flags & GPIOLINE_FLAG_OPEN_DRAIN);
        cfgOpenSource = !!(linfo.flags & GPIOLINE_FLAG_OPEN_SOURCE);

        success = true;
        break;
      }
    }

    if (close(fd) == -1)
      perror("Failed to close GPIO character device file");

  } while (false);

  return success;
}

IoPinGpioUtilsLinux48& IoPinGpioUtilsLinux48::findPin(::std::string const& name)
{
  if (chipName.empty())
  {
    do
    {
      const struct dirent *ent;
      DIR *dp;

      /* go through all GPIO devices one at a time and look for the GPIO line */
      dp = opendir(basePath.c_str());
      if (!dp)
      {
        myErrno = errno;
        perror("scanning devices: Failed to open directory");
        break;
      }

      while (ent = readdir(dp), ent) {
        if (check_prefix(ent->d_name, "gpiochip")) {

          //  got through all GPIO lines and compare for the desired name
          if (locateNameInChip(ent->d_name, name))
            break;
        }
      }

      if (closedir(dp) == -1)
      {
        myErrno = errno;
        perror("scanning devices: Failed to close directory");
        break;
      }

    } while (false);
  }
  else
    locateNameInChip(chipName, name);


  return *this;
}

IoPinGpioUtilsLinux48& IoPinGpioUtilsLinux48::prepareAsOpenDrain(bool openDrain)
{
  cfgOpenDrain = openDrain;
  if (openDrain)
    cfgOpenSource = false;
  return *this;
}

IoPinGpioUtilsLinux48& IoPinGpioUtilsLinux48::prepareAsOpenSource(bool openSource)
{
  cfgOpenSource = openSource;
  if (openSource)
    cfgOpenDrain = false;
  return *this;
}

IoPinGpioUtilsLinux48& IoPinGpioUtilsLinux48::prepareAsActiveHigh(bool activeHigh)
{
  cfgActiveHigh = activeHigh;
  return *this;
}

void IoPinGpioUtilsLinux48::configureAsIs()
{
  // intentionally do nothing more than request the line
  requestLine(true);
}

void IoPinGpioUtilsLinux48::configureAsInput()
{
  cfgInput = true;
  requestLine();
}

void IoPinGpioUtilsLinux48::configureAsOutput(bool setHigh)
{
  cfgInput = false;
  outputIsActive = setHigh;
  requestLine();
}

bool IoPinGpioUtilsLinux48::isActive()
{
  assert(-1 < handleFd);

  bool active = false;

  struct gpiohandle_data data;

  auto ret = gpiotools_get_values(handleFd, &data);

  if (ret)
    myErrno = -ret;
  else
    active = !!data.values[0];

  return active;
}

bool IoPinGpioUtilsLinux48::isPassive()
{
  return !isActive();
}

void IoPinGpioUtilsLinux48::activate()
{
  assert(-1 < handleFd);

  struct gpiohandle_data data;

  data.values[0] = 1;
  auto ret = gpiotools_set_values(handleFd, &data);

  if (ret)
    myErrno = -ret;
  else
    outputIsActive = true;
}

void IoPinGpioUtilsLinux48::deactivate()
{
  assert(-1 < handleFd);

  struct gpiohandle_data data;

  data.values[0] = 0;
  auto ret = gpiotools_set_values(handleFd, &data);

  if (ret)
    myErrno = -ret;
  else
  outputIsActive = false;
}


bool IoPinGpioUtilsLinux48::isInput() const
{
  return cfgInput;
}

bool IoPinGpioUtilsLinux48::isOpenDrain() const
{
  return cfgOpenDrain;
}

bool IoPinGpioUtilsLinux48::isOpenSource() const
{
  return cfgOpenSource;
}

bool IoPinGpioUtilsLinux48::isPushPull() const
{
  return (!isInput() && (!isOpenDrain() && !isOpenSource()));
}

bool IoPinGpioUtilsLinux48::hasLine() const
{
  return isRequested;
}

void IoPinGpioUtilsLinux48::release()
{
  if (handleFd > -1)
  {
    auto ret = gpiotools_release_linehandle(handleFd);
    if (ret)
      myErrno = -ret;
  }
  handleFd = -1;
}

void IoPinGpioUtilsLinux48::requestLine(bool asIs)
{
  if (isRequested)
    release();

  struct gpiohandle_data defaultData;
  unsigned int flags;

  consumer.resize(32);

  defaultData.values[0] = outputIsActive ? 1 : 0;

  flags = 0;

  flags = (asIs ? 0 : cfgInput ? GPIOHANDLE_REQUEST_INPUT : GPIOHANDLE_REQUEST_OUTPUT);

  if (not cfgActiveHigh)
    flags |= GPIOHANDLE_REQUEST_ACTIVE_LOW;

  if (cfgOpenSource)
    flags |= GPIOHANDLE_REQUEST_OPEN_SOURCE;
  else if (cfgOpenDrain)
    flags |= GPIOHANDLE_REQUEST_OPEN_DRAIN;

  handleFd = gpiotools_request_linehandle(chipName.c_str(), &lineIndex, 1, flags, &defaultData, consumer.c_str());

  isRequested = (handleFd > -1);

  // update Konfigurationsbits
  if (isRequested)
  {
    struct gpiochip_info cinfo;
    char *chrdev_name;
    int ret;

    ret = asprintf(&chrdev_name, "/dev/%s", chipName.c_str());
    if (ret < 0)
    {
      myErrno = errno;
      return;
    }

    int fd;

    fd = open(chrdev_name, 0);
    if (fd == -1) {
      myErrno = errno;
      fprintf(stderr, "Failed to open %s\n", chrdev_name);
      goto exit_close_error;
    }

    /* Inspect this GPIO chip */
    ret = ioctl(fd, GPIO_GET_CHIPINFO_IOCTL, &cinfo);
    if (ret == -1) {
      myErrno = errno;
      perror("Failed to issue CHIPINFO IOCTL\n");
      goto exit_close_error;
    }

    struct gpioline_info linfo;

    memset(&linfo, 0, sizeof(linfo));
    linfo.line_offset = lineIndex;

    ret = ioctl(fd, GPIO_GET_LINEINFO_IOCTL, &linfo);
    if (ret == -1) {
      myErrno = errno;
      perror("Failed to issue LINEINFO IOCTL\n");
      goto exit_close_error;
    }

    cfgInput = !(linfo.flags & GPIOLINE_FLAG_IS_OUT);
    cfgActiveHigh = !(linfo.flags & GPIOLINE_FLAG_ACTIVE_LOW);
    cfgOpenDrain = !!(linfo.flags & GPIOLINE_FLAG_OPEN_DRAIN);
    cfgOpenSource = !!(linfo.flags & GPIOLINE_FLAG_OPEN_SOURCE);

    exit_close_error:
    if (close(fd) == -1)
      perror("Failed to close GPIO character device file");
    free(chrdev_name);
  }
  else
  {
    myErrno = -handleFd;
    handleFd = -1;
  }
}

bool IoPinGpioUtilsLinux48::getPosixError(const char* &text, int& errNo)
{
  bool hasError = hasPosixError();

  if (hasError)
  {
    errNo = myErrno;
    text = strerror(myErrno);
  }

  return hasError;
}

} /* namespace Hardware */
