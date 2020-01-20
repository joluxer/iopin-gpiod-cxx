/*
 * IoPin.h
 *
 */

#ifndef IOPIN_H_
#define IOPIN_H_

class IoPin
{
public:
  virtual
  ~IoPin();

  virtual bool isActive();
  virtual bool isPassive();
  virtual void activate();       ///< high active pins go high, low active pins go low
  virtual void deactivate();   ///< high active pins go low, low active pins go high

  IoPin(const IoPin&) = delete;
  IoPin& operator=(const IoPin&) = delete;
protected:
  IoPin() = default;
};

struct NonVirtualIoPinBase
{
  /*
   * Diese Klasse ist für Template-Spezialisierungen, welche keine virtuelle
   * Basis benötigen, weil immer die finale Klasse refenrenziert wird.
   */
};

#endif /* IOPIN_H_ */
