#ifndef multinomial_h
#define multinomial_h

#include "doublevector.h"

class Multinomial;

/**
 * \class Multinomial
 * \brief This is the class that calculates a log likelihood score by comparing 2 vectors based on a multinomial distribution
 */
class Multinomial {
public:
  /**
   * \brief This is the default Multinomial constructor
   */
  Multinomial(double value) { bigvalue = value; loglikelihood = 0.0; };
  /**
   * \brief This is the default Multinomial destructor
   */
  ~Multinomial() {};
  /**
   * \brief This is the function that calculates a log likelihood score by comparing 2 vectors based on a multinomial distribution
   * \param data is the DoubleVector containing the input data
   * \param dist is the DoubleVector containing the modelled data
   * \return likelihood
   */
  double CalcLogLikelihood(const DoubleVector& data, const DoubleVector& dist);
  /**
   * \brief This will return the log likelihood score
   * \return loglikelihood
   */
  double returnLogLikelihood() const { return loglikelihood; };
protected:
  /**
   * \brief This is used to calculate the default minimum probability for unlikely values
   */
  double bigvalue;
  /**
   * \brief This is the log likelihood score
   */
  double loglikelihood;
};

#endif
