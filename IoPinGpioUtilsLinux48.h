/*
 * IoPinGpioUtilsLinux48.h
 *
 *  Created on: 17.01.2020
 *      Author: lode
 */

#ifndef IOPINGPIOUTILSLINUX48_H_
#define IOPINGPIOUTILSLINUX48_H_

#include "Interface/IoPin.h"

#include <string>

namespace Hardware
{

/*
 *
 */
class IoPinGpioUtilsLinux48: public IoPin
{
public:
  IoPinGpioUtilsLinux48(const char* consumer = nullptr, const char* pinName = nullptr, const char* gpioChip = nullptr);
  IoPinGpioUtilsLinux48(const char* consumer, const char* gpioChip, int pinOffset);
  virtual
  ~IoPinGpioUtilsLinux48();

  static
  void setDevPath(const char*);

  static
  void setGlobalConsumer(const char* consumer);

  void setConsumer(const ::std::string& consumer);

  /**
   * Find the GPIO-Chip by name or path
   * @param device
   * @return this
   */
  IoPinGpioUtilsLinux48& inPort(::std::string const& device);

  /**
   * Set the GPIO line in the port by offset.
   * @param offset
   * @return this
   */
  IoPinGpioUtilsLinux48& atPin(unsigned int offset);

  /**
   * Find the GPIO line globally (without port selection) or
   * locally (on the selected port) by its name.
   * @param name
   * @return this
   */
  IoPinGpioUtilsLinux48& findPin(::std::string const& name);

  /**
   * Prepare the allocation as active-high or active-low line.
   * Actually if not prepeared at all with this call, it is active high either,
   * use this call explicit to prepare an active low line.
   * @param activeHigh
   * @return
   */
  IoPinGpioUtilsLinux48& prepareAsActiveHigh(bool activeHigh = true);

  /**
   * Prepare the allocation as open drain line.
   * @param activeHigh
   * @return
   */
  IoPinGpioUtilsLinux48& prepareAsOpenDrain(bool openDrain = true);

  /**
   * Prepare the allocation as open source line.
   * @param activeHigh
   * @return
   */
  IoPinGpioUtilsLinux48& prepareAsOpenSource(bool openSource = true);

  /**
   * Request the line, do not change direction, open source/drain/push-pull.
   * If already requested, do nothing.
   * Later may be checked for the direction and other attributes.
   */
  void configureAsIs();

  /**
   * Request as input line.
   * If already requested as output, release and request again.
   * This is not atomic, so you might hit the rare case, where you loose the line completly.
   * @return this
   */
  void configureAsInput();

  /**
   * Request as output line.
   * If already requested as input, release and request again.
   * This is not atomic, so you might hit the rare case, where you loose the line completly.
   * @param setHigh  Set the line high (or low) on successful allocation.
   * @return
   */
  void configureAsOutput(bool setHigh = false);

  bool isActive() override;     ///< high active pins are high, low active pins are low
  bool isPassive() override;    ///< high active pins are low, low active pins are high
  void activate() override;     ///< high active pins go high, low active pins go low
  void deactivate() override;   ///< high active pins go low, low active pins go high

  /**
   * Check the activity level for the line.
   * @return true, if the line is active high, else false for active low.
   */
  bool isActiveHigh() const;

  /**
   * Check the direction of the line for input.
   * @return true, if the line is input.
   */
  bool isInput() const;

  /**
   * Check the output type of the line.
   * @return true, if the line is open drain output.
   */
  bool isOpenDrain() const;

  /**
   * Check the output type of the line.
   * @return true, if the line is open source output.
   */
  bool isOpenSource() const;

  /**
   * Check the output type of the line.
   * @return true, if the line is push-pull output.
   */
  bool isPushPull() const;

  /**
   * Check for the successful request/allocation of the GPIO line.
   * @return true, if the GPIO line is found, allocated and usable.
   */
  bool hasLine() const;

  /**
   * Release the system resources and the requested line for future use by this or some other consumer.
   */
  void release();

  bool hasPosixError();

  bool getPosixError(const char* &text, int& errNo);

protected:
  ::std::string consumer;
  ::std::string chipName;
  ::std::string lineName;
  unsigned lineIndex;
  int handleFd;
  int myErrno;

  mutable bool cfgInput:1;
  mutable bool cfgActiveHigh:1;
  mutable bool cfgOpenDrain:1;
  mutable bool cfgOpenSource:1;
  mutable bool isRequested:1;
  bool outputIsActive:1;

  bool locateNameInChip(::std::string const& chipName, ::std::string const& name);
  void requestLine(bool asIs = false);

  static ::std::string globalConsumer;
  static ::std::string basePath;
};


inline
bool IoPinGpioUtilsLinux48::hasPosixError()
{
  return (0 != myErrno);
}

} /* namespace Hardware */

#endif /* IOPINGPIOUTILSLINUX48_H_ */
