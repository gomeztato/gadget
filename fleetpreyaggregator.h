#ifndef fleetpreyaggregator_h
#define fleetpreyaggregator_h

#include "areatime.h"
#include "conversionindex.h"
#include "agebandmatrix.h"
#include "agebandmatrixptrvector.h"
#include "charptrvector.h"
#include "fleetptrvector.h"
#include "stockptrvector.h"
#include "intmatrix.h"
#include "conversionindexptrvector.h"

class FleetPreyAggregator;

class FleetPreyAggregator {
public:
  /**
   * \brief This is the FleetPreyAggregator constructor
   * \param Fleets is the FleetPtrVector of the fleets that will be aggregated
   * \param Stocks is the StockPtrVector of the stocks that will be aggregated
   * \param Lgrpdiv is the LengthGroupDivision of the stocks
   * \param Areas is the IntMatrix of areas that the catch can take place on
   * \param Ages is the IntMatrix of ages of the stocks
   * \param overcons is a flag to determine whether to calculate overconsumption of the stocks
   */
  FleetPreyAggregator(const FleetPtrVector& Fleets, const StockPtrVector& Stocks,
    LengthGroupDivision* const Lgrpdiv, const IntMatrix& Areas,
    const IntMatrix& Ages, const int overcons);
  /**
   * \brief This is the default FleetPreyAggregator destructor
   */
  ~FleetPreyAggregator();
  /**
   * \brief This function will return the number of length groups of the aggregator
   * \return number of length groups
   */
  int NoLengthGroups() const { return LgrpDiv->NoLengthGroups(); };
  /**
   * \brief This function will return the number of age groups of the aggregator
   * \return number of age groups
   */
  int NoAgeGroups() const { return ages.Nrow(); };
  /**
   * \brief This function will return the LengthGroupDivision of the aggregator
   * \return LgrpDiv, the LengthGroupDivision of the aggregator
   */
  LengthGroupDivision* returnLengthGroupDiv() const { return LgrpDiv; };
  /**
   * \brief This function will aggregate the fleet catch data
   * \param TimeInfo is the TimeClass for the current model
   */
  void Sum(const TimeClass* const TimeInfo);
  /**
   * \brief This function will aggregate the fleet catch data for a fleksibest-style mortality model
   * \param TimeInfo is the TimeClass for the current model
   * \param dummy is a flag to denote that this function is to be used with a fleksibest-style mortality model
   */
  void Sum(const TimeClass* const TimeInfo, int dummy);
  /**
   * \brief This will print the aggregator information to the ofstream specified
   * \param outfile is the ofstream to the file that the aggregator information gets sent to
   */
  void Print(ofstream &outfile) const;
  /**
   * \brief This will return the AgeBandMatrixPtrVector containing the aggregated catch information
   * \return total, the AgeBandMatrixPtrVector of aggregated catch information
   */
  const AgeBandMatrixPtrVector& returnSum() const { return total; };
  const IntVector& getMinCol() const { return mincol; };
  const IntVector& getMaxCol() const { return maxcol; };
  int getMinRow() const { return minrow; };
  int getMaxRow() const { return maxrow; };
private:
  /**
   * \brief This is the FleetPtrVector of the fleets that will be aggregated
   */
  FleetPtrVector fleets;
  /**
   * \brief This is the StockPtrVector of the stocks that will be aggregated
   */
  StockPtrVector stocks;
  /**
   * \brief This is the LengthGroupDivision of the aggregated stocks
   */
  LengthGroupDivision* LgrpDiv;
  /**
   * \brief This is the IntMatrix used to store area information
   */
  IntMatrix areas;
  /**
   * \brief This is the IntMatrix used to store age information
   */
  IntMatrix ages;
  IntVector mincol;
  IntVector maxcol;
  int minrow;
  int maxrow;
  /**
   * \brief This is the flag used to denote whether to calculate overconsumption of the stocks or not
   */
  int overconsumption;
  ConversionIndexPtrVector CI;
  /**
   * \brief This is the AgeBandMatrixPtrVector used to store the aggregated catch
   */
  AgeBandMatrixPtrVector total;
};

#endif
