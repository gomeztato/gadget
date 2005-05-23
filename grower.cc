#include "grower.h"
#include "errorhandler.h"
#include "readfunc.h"
#include "readword.h"
#include "keeper.h"
#include "areatime.h"
#include "growthcalc.h"
#include "gadget.h"

extern ErrorHandler handle;

Grower::Grower(CommentStream& infile, const LengthGroupDivision* const OtherLgrpDiv,
  const LengthGroupDivision* const GivenLgrpDiv, const IntVector& Areas,
  const TimeClass* const TimeInfo, Keeper* const keeper, const char* refWeight,
  const AreaClass* const Area, const CharPtrVector& lenindex)
  : LivesOnAreas(Areas), LgrpDiv(0), CI(0), growthcalc(0) {

  char text[MaxStrLength];
  strncpy(text, "", MaxStrLength);
  int i;

  keeper->addString("grower");
  fixedweights = 0;
  LgrpDiv = new LengthGroupDivision(*GivenLgrpDiv);
  CI = new ConversionIndex(OtherLgrpDiv, LgrpDiv, 1);

  readWordAndValue(infile, "growthfunction", text);
  if (strcasecmp(text, "multspec") == 0) {
    functionnumber = 1;
    growthcalc = new GrowthCalcA(infile, areas, TimeInfo, keeper);
  } else if (strcasecmp(text, "fromfile") == 0) {
    functionnumber = 2;
    growthcalc = new GrowthCalcB(infile, areas, TimeInfo, keeper, Area, lenindex);
  } else if (strcasecmp(text, "weightvb") == 0) {
    functionnumber = 3;
    growthcalc = new GrowthCalcC(infile, areas, TimeInfo, LgrpDiv, keeper, refWeight);
  } else if (strcasecmp(text, "weightjones") == 0) {
    functionnumber = 4;
    growthcalc = new GrowthCalcD(infile, areas, TimeInfo, LgrpDiv, keeper, refWeight);
  } else if (strcasecmp(text, "weightvbexpanded") == 0) {
    functionnumber = 5;
    growthcalc = new GrowthCalcE(infile, areas, TimeInfo, LgrpDiv, keeper, refWeight);
  } else if (strcasecmp(text, "lengthvb") == 0) {
    functionnumber = 6;
    fixedweights = 1;
    growthcalc = new GrowthCalcF(infile, areas, TimeInfo, keeper, Area, lenindex);
  } else if (strcasecmp(text, "lengthpower") == 0) {
    functionnumber = 7;
    fixedweights = 1;
    growthcalc = new GrowthCalcG(infile, areas, TimeInfo, keeper, Area, lenindex);
  } else if (strcasecmp(text, "lengthvbsimple") == 0) {
    functionnumber = 8;
    growthcalc = new GrowthCalcH(infile, areas, TimeInfo, keeper);
  } else {
    handle.logFileMessage(LOGFAIL, "Error in stock file - unrecognised growth function", text);
  }

  infile >> ws >>  text;
  if (strcasecmp(text, "beta") == 0) {
    //Beta binomial growth distribution code is used
    infile >> beta;
    beta.Inform(keeper);
    readWordAndVariable(infile, "maxlengthgroupgrowth", maxlengthgroupgrowth);

    part1.resize(maxlengthgroupgrowth + 1, 0.0);
    part2.resize(maxlengthgroupgrowth + 1, 0.0);
    part4.resize(maxlengthgroupgrowth + 1, 0.0);

  } else if (strcasecmp(text, "meanvarianceparameters") == 0) {
    handle.logFileMessage(LOGFAIL, "The mean variance parameters implementation of the growth is no longer supported\nUse the beta-binomial distribution implementation of the growth instead");

  } else
    handle.logFileUnexpected(LOGFAIL, "beta", text);

  //Finished reading from input files.
  keeper->clearLast();
  int noareas = areas.Size();
  int len = LgrpDiv->numLengthGroups();
  int otherlen = OtherLgrpDiv->numLengthGroups();
  PopInfo nullpop;
  numGrow.AddRows(noareas, len, nullpop);

  //setting storage spaces for growth
  calcLengthGrowth.AddRows(noareas, len, 0.0);
  calcWeightGrowth.AddRows(noareas, len, 0.0);
  interpLengthGrowth.AddRows(noareas, otherlen, 0.0);
  interpWeightGrowth.AddRows(noareas, otherlen, 0.0);
  lgrowth.resize(noareas);
  wgrowth.resize(noareas);
  for (i = 0; i < noareas; i++) {
    lgrowth[i] = new DoubleMatrix(maxlengthgroupgrowth + 1, otherlen, 0.0);
    wgrowth[i] = new DoubleMatrix(maxlengthgroupgrowth + 1, otherlen, 0.0);
  }
  dummyfphi.resize(len, 0.0);
}

Grower::~Grower() {
  int i;
  for (i = 0; i < lgrowth.Size(); i++) {
    delete lgrowth[i];
    delete wgrowth[i];
  }
  delete CI;
  delete LgrpDiv;
  delete growthcalc;
}

void Grower::Print(ofstream& outfile) const {
  int i, j, area;

  outfile << "\nGrower\n\t";
  LgrpDiv->Print(outfile);
  for (area = 0; area < areas.Size(); area++) {
    outfile << "\tLength increase on internal area " << areas[area] << ":\n\t";
    for (i = 0; i < calcLengthGrowth.Ncol(area); i++)
      outfile << sep << calcLengthGrowth[area][i];
    outfile << "\n\tWeight increase on internal area " << areas[area] << ":\n\t";
    for (i = 0; i < calcWeightGrowth.Ncol(area); i++)
      outfile << sep << calcWeightGrowth[area][i];
    outfile << "\n\tDistributed length increase on internal area " << areas[area] << ":\n";
    for (i = 0; i < lgrowth[area]->Nrow(); i++) {
      outfile << TAB;
      for (j = 0; j < lgrowth[area]->Ncol(i); j++)
        outfile << sep << (*lgrowth[area])[i][j];
      outfile << endl;
    }
    outfile << "\tDistributed weight increase on internal area " << areas[area] << ":\n";
    for (i = 0; i < wgrowth[area]->Nrow(); i++) {
      outfile << TAB;
      for (j = 0; j < wgrowth[area]->Ncol(i); j++)
        outfile << sep << (*wgrowth[area])[i][j];
      outfile << endl;
    }
  }
}

//The following function is just a copy of Prey::Sum
void Grower::Sum(const PopInfoVector& NumberInArea, int area) {
  int i, inarea = this->areaNum(area);
  for (i = 0; i < numGrow[inarea].Size(); i++)
    numGrow[inarea][i].N = 0.0;
  numGrow[inarea].Sum(&NumberInArea, *CI);
}

void Grower::GrowthCalc(int area,
  const AreaClass* const Area, const TimeClass* const TimeInfo) {

  this->GrowthCalc(area, Area, TimeInfo, dummyfphi, dummyfphi);
}

void Grower::GrowthCalc(int area,
  const AreaClass* const Area, const TimeClass* const TimeInfo,
  const DoubleVector& FPhi, const DoubleVector& MaxCon) {

  int inarea = this->areaNum(area);
  growthcalc->GrowthCalc(area, calcLengthGrowth[inarea], calcWeightGrowth[inarea],
    numGrow[inarea], Area, TimeInfo, FPhi, MaxCon, LgrpDiv);

  CI->interpolateLengths(interpLengthGrowth[inarea], calcLengthGrowth[inarea]);
  CI->interpolateLengths(interpWeightGrowth[inarea], calcWeightGrowth[inarea]);
}

const DoubleMatrix& Grower::getLengthIncrease(int area) const {
  return *lgrowth[this->areaNum(area)];
}

const DoubleMatrix& Grower::getWeightIncrease(int area) const {
  return *wgrowth[this->areaNum(area)];
}

const DoubleVector& Grower::getWeight(int area) const {
  return interpWeightGrowth[this->areaNum(area)];
}

double Grower::getPowerValue() {
  return growthcalc->getPower();
}

double Grower::getMultValue() {
  return growthcalc->getMult();
}

void Grower::Reset() {
  int i, j, area;
  double factorialx, tmppart, tmpmax;

  for (area = 0; area < areas.Size(); area++) {
    for (i = 0; i < LgrpDiv->numLengthGroups(); i++) {
      calcLengthGrowth[area][i] = 0.0;
      calcWeightGrowth[area][i] = 0.0;
      numGrow[area][i].N = 0.0;
      numGrow[area][i].W = 0.0;
    }

    for (i = 0; i < wgrowth[area]->Nrow(); i++) {
      for (j = 0; j < wgrowth[area]->Ncol(i); j++) {
        (*wgrowth[area])[i][j] = 0.0;
        (*lgrowth[area])[i][j] = 0.0;
      }
    }

    for (i = 0; i < interpLengthGrowth.Ncol(); i++) {
      interpLengthGrowth[area][i] = 0.0;
      interpWeightGrowth[area][i] = 0.0;
    }
  }

  tmpmax = double(maxlengthgroupgrowth) * 1.0;
  part1[0] = 1.0;
  part1[1] = tmpmax;
  factorialx = 1.0;
  tmppart = tmpmax;
  if (maxlengthgroupgrowth > 1) {
    for (i = 2; i < maxlengthgroupgrowth + 1; i++) {
      tmppart *= (tmpmax - i + 1);
      factorialx *= double(i);
      part1[i] = (tmppart / factorialx);
    }
  }

  part2[maxlengthgroupgrowth] = 1.0;
  part2[maxlengthgroupgrowth - 1] = beta;
  if (maxlengthgroupgrowth > 1)
    for (i = maxlengthgroupgrowth - 2; i >= 0; i--)
      part2[i] = part2[i + 1] * (beta + tmpmax - i - 1);

  if (handle.getLogLevel() >= LOGMESSAGE)
    handle.logMessage(LOGMESSAGE, "Reset grower data");
}
