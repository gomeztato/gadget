#include "maturity.h"
#include "stock.h"
#include "mathfunc.h"
#include "readfunc.h"
#include "readword.h"
#include "conversionindex.h"
#include "errorhandler.h"
#include "gadget.h"

// ********************************************************
// Functions for base Maturity
// ********************************************************
Maturity::Maturity(const IntVector& tmpareas, int minage, const IntVector& minlength,
  const IntVector& size, const LengthGroupDivision* const lgrpdiv)
  : LivesOnAreas(tmpareas), LgrpDiv(new LengthGroupDivision(*lgrpdiv)),
    Storage(tmpareas.Size(), minage, minlength, size),
    TagStorage(tmpareas.Size(), minage, minlength, size) {

}

Maturity::~Maturity() {
  int i;
  for (i = 0; i < MatureStockNames.Size(); i++)
    delete[] MatureStockNames[i];
  for (i = 0; i < CI.Size(); i++)
    delete CI[i];
  delete LgrpDiv;
}

void Maturity::setStock(StockPtrVector& stockvec) {
  int index = 0;
  int i, j;
  DoubleVector tmpratio;

  for (i = 0; i < stockvec.Size(); i++)
    for (j = 0; j < MatureStockNames.Size(); j++)
      if (strcasecmp(stockvec[i]->Name(), MatureStockNames[j]) == 0) {
        MatureStocks.resize(1);
        tmpratio.resize(1);
        MatureStocks[index] = stockvec[i];
        tmpratio[index] = Ratio[j];
        index++;
      }

  if (index != MatureStockNames.Size()) {
    cerr << "Error - did not find the stock(s) matching:\n";
    for (i = 0; i < MatureStockNames.Size(); i++)
      cerr << (const char*)MatureStockNames[i] << sep;
    cerr << "\nwhen searching for mature stock(s) - found only:\n";
    for (i = 0; i < stockvec.Size(); i++)
      cerr << stockvec[i]->Name() << sep;
    cerr << endl;
    exit(EXIT_FAILURE);
  }

  for (i = Ratio.Size(); i > 0; i--)
    Ratio.Delete(0);
  Ratio.resize(tmpratio.Size());
  for (i = 0; i < tmpratio.Size(); i++)
    Ratio[i] = tmpratio[i];

  CI.resize(MatureStocks.Size(), 0);
  for (i = 0; i < MatureStocks.Size(); i++)
    CI[i] = new ConversionIndex(LgrpDiv, MatureStocks[i]->returnLengthGroupDiv());

  for (i = 0; i < MatureStocks.Size(); i++) {
    index = 0;
    for (j = 0; j < areas.Size(); j++)
      if (!MatureStocks[i]->IsInArea(areas[j]))
        index++;

    if (index != 0)
      cerr << "Warning - maturation requested to stock " << (const char*)MatureStocks[i]->Name()
        << "\nwhich might not be defined on " << index << " areas\n";
  }
}

void Maturity::Print(ofstream& outfile) const {
  int i;
  outfile << "\nMaturity\n\tRead names of mature stocks:";
  for (i = 0; i < MatureStockNames.Size(); i++)
    outfile << sep << (const char*)(MatureStockNames[i]);
  outfile << "\n\tNames of mature stocks (through pointers):";
  for (i = 0; i < MatureStocks.Size(); i++)
    outfile << sep << (const char*)(MatureStocks[i]->Name());
  outfile << "\n\tStored numbers:\n";
  for (i = 0; i < areas.Size(); i++) {
    outfile << "\tInternal area " << areas[i] << endl;
    Storage[i].PrintNumbers(outfile);
  }
}

void Maturity::Move(int area, const TimeClass* const TimeInfo) {
  assert(this->IsMaturationStep(area, TimeInfo));
  int i, inarea = AreaNr[area];
  for (i = 0; i < MatureStocks.Size(); i++) {
    MatureStocks[i]->Add(Storage[inarea], CI[i], area, Ratio[i],
      Storage[inarea].Minage(), Storage[inarea].Maxage());

    if (TagStorage.NrOfTagExp() > 0)
      MatureStocks[i]->Add(TagStorage, inarea, CI[i], area, Ratio[i],
        TagStorage[inarea].Minage(), TagStorage[inarea].Maxage());
  }
  Storage[inarea].setToZero();
  TagStorage[inarea].setToZero();
}

void Maturity::PutInStorage(int area, int age, int length, double number,
  double weight, const TimeClass* const TimeInfo) {

  int inarea = AreaNr[area];
  assert(this->IsMaturationStep(area, TimeInfo));
  if (isZero(number) || isZero(weight)) {
    Storage[inarea][age][length].N = 0.0;
    Storage[inarea][age][length].W = 0.0;
  } else {
    Storage[inarea][age][length].N = number;
    Storage[inarea][age][length].W = weight;
  }
}

void Maturity::PutInStorage(int area, int age, int length, double number,
  const TimeClass* const TimeInfo, int id) {

  assert(this->IsMaturationStep(area, TimeInfo));
  if (TagStorage.NrOfTagExp() <= 0) {
    cerr << "Error in tagging maturity - no tagging experiment\n";
    exit(EXIT_FAILURE);
  } else if ((id >= TagStorage.NrOfTagExp()) || (id < 0)) {
    cerr << "Error in tagging maturity - illegal tagging experiment id\n";
    exit(EXIT_FAILURE);
  } else
    *(TagStorage[AreaNr[area]][age][length][id].N) = (number > 0.0 ? number: 0.0);
}

void Maturity::Precalc(const TimeClass* const TimeInfo) {
  int area, age, l, tag;
  if (TimeInfo->CurrentTime() == 1) {
    for (area = 0; area < areas.Size(); area++) {
      for (age = Storage[area].Minage(); age <= Storage[area].Maxage(); age++) {
        for (l = Storage[area].Minlength(age); l < Storage[area].Maxlength(age); l++) {
          Storage[area][age][l].N = 0.0;
          Storage[area][age][l].W = 0.0;
          for (tag = 0; tag < TagStorage.NrOfTagExp(); tag++)
            *(TagStorage[area][age][l][tag].N) = 0.0;
        }
      }
    }
  }
}

const StockPtrVector& Maturity::getMatureStocks() {
  return MatureStocks;
}

void Maturity::addMaturityTag(const char* tagname) {
  TagStorage.addTag(tagname);
}

void Maturity::deleteMaturityTag(const char* tagname) {
  int minage, maxage, minlen, maxlen, age, length, i;
  int id = TagStorage.getID(tagname);

  if (id >= 0) {
    minage = TagStorage[0].Minage();
    maxage = TagStorage[0].Maxage();
    // Remove allocated memory
    for (i = 0; i < TagStorage.Size(); i++) {
      for (age = minage; age <= maxage; age++) {
        minlen = TagStorage[i].Minlength(age);
        maxlen = TagStorage[i].Maxlength(age);
        for (length = minlen; length < maxlen; length++) {
          delete[] (TagStorage[i][age][length][id].N);
          (TagStorage[i][age][length][id].N) = NULL;
        }
      }
    }
    TagStorage.deleteTag(tagname);

  } else {
    cerr << "Error in tagging maturity - trying to delete tag with name "
      << tagname << " but there is no tag with that name\n";
  }
}

// ********************************************************
// Functions for MaturityA
// ********************************************************
MaturityA::MaturityA(CommentStream& infile, const TimeClass* const TimeInfo,
  Keeper* const keeper, int minage, const IntVector& minabslength, const IntVector& size,
  const IntVector& tmpareas, const LengthGroupDivision* const lgrpdiv, int NoMatconst)
  : Maturity(tmpareas, minage, minabslength, size, lgrpdiv), PrecalcMaturation(minabslength, size, minage) {

  ErrorHandler handle;
  char text[MaxStrLength];
  strncpy(text, "", MaxStrLength);
  int i;
  keeper->AddString("maturity");
  infile >> text;
  if ((strcasecmp(text, "nameofmaturestocksandratio") == 0) || (strcasecmp(text, "maturestocksandratios") == 0)) {
    infile >> text;
    i = 0;
    while (strcasecmp(text, "coefficients") != 0 && infile.good()) {
      MatureStockNames.resize(1);
      MatureStockNames[i] = new char[strlen(text) + 1];
      strcpy(MatureStockNames[i], text);
      Ratio.resize(1);
      infile >> Ratio[i];
      i++;
      infile >> text;
    }
  } else
    handle.Unexpected("maturestocksandratios", text);

  if (!infile.good())
    handle.Failure("coefficients");
  maturityParameters.resize(NoMatconst, keeper);
  maturityParameters.read(infile, TimeInfo, keeper);
  keeper->ClearLast();
}

void MaturityA::Precalc(const TimeClass* const TimeInfo) {
  this->Maturity::Precalc(TimeInfo);
  maturityParameters.Update(TimeInfo);
  double my;
  int age, len;

  if (maturityParameters.DidChange(TimeInfo)) {
    for (age = PrecalcMaturation.Minage(); age <= PrecalcMaturation.Maxage(); age++) {
      for (len = PrecalcMaturation.Minlength(age); len < PrecalcMaturation.Maxlength(age); len++) {
        if ((age >= minMatureAge) && (len >= minMatureLength)) {
          my = exp(-maturityParameters[0] * (LgrpDiv->Meanlength(len) - maturityParameters[1])
                 - maturityParameters[2] * (age - maturityParameters[3]));
          PrecalcMaturation[age][len] = 1 / (1 + my);
        } else
          PrecalcMaturation[age][len] = 0.0;
      }
    }
  }
}

void MaturityA::setStock(StockPtrVector& stockvec) {
  this->Maturity::setStock(stockvec);

  int i;
  minMatureAge = 9999;
  double minlength = 9999.0;
  for (i = 0; i < MatureStocks.Size(); i++) {
    if (MatureStocks[i]->Minage() < minMatureAge)
      minMatureAge = MatureStocks[i]->Minage();
    if (MatureStocks[i]->returnLengthGroupDiv()->minLength() < minlength)
      minlength = MatureStocks[i]->returnLengthGroupDiv()->minLength();
  }
  minMatureLength = LgrpDiv->NoLengthGroup(minlength);
}

double MaturityA::MaturationProbability(int age, int length, int growth,
  const TimeClass* const TimeInfo, int area, double weight) {

  if ((age >= minMatureAge) && (length >= minMatureLength)) {
    double prob = PrecalcMaturation[age][length] * (maturityParameters[0] * growth * LgrpDiv->dl()
                  + maturityParameters[2] * TimeInfo->LengthOfCurrent() / TimeInfo->LengthOfYear());
    return (min(max(0.0, prob), 1.0));
  }
  return 0.0;
}

void MaturityA::Print(ofstream& outfile) const {
  Maturity::Print(outfile);
  outfile << "\tPrecalculated maturity:\n";
  PrecalcMaturation.Print(outfile);
}

int MaturityA::IsMaturationStep(int area, const TimeClass* const TimeInfo) {
  return 1;
}

// ********************************************************
// Functions for MaturityB
// ********************************************************
MaturityB::MaturityB(CommentStream& infile, const TimeClass* const TimeInfo,
  Keeper* const keeper, int minage, const IntVector& minabslength,
  const IntVector& size, const IntVector& tmpareas, const LengthGroupDivision* const lgrpdiv)
  : Maturity(tmpareas, minage, minabslength, size, lgrpdiv) {

  ErrorHandler handle;
  char text[MaxStrLength];
  strncpy(text, "", MaxStrLength);
  int i;
  infile >> text;
  keeper->AddString("maturity");
  if ((strcasecmp(text, "nameofmaturestocksandratio") == 0) || (strcasecmp(text, "maturestocksandratios") == 0)) {
    infile >> text;
    i = 0;
    while (!(strcasecmp(text, "maturitysteps") == 0) && infile.good()) {
      MatureStockNames.resize(1);
      MatureStockNames[i] = new char[strlen(text) + 1];
      strcpy(MatureStockNames[i], text);
      Ratio.resize(1);
      infile >> Ratio[i];
      i++;
      infile >> text;
    }
  } else
    handle.Unexpected("maturestocksandratios", text);

  if (!infile.good())
    handle.Failure("maturitysteps");
  infile >> ws;
  while (isdigit(infile.peek()) && !infile.eof()) {
    maturitystep.resize(1);
    infile >> maturitystep[maturitystep.Size() - 1] >> ws;
  }
  infile >> text;
  if (!(strcasecmp(text, "maturitylengths") == 0))
    handle.Unexpected("maturitylengths", text);
  while (maturitylength.Size() < maturitystep.Size() && !infile.eof()) {
    maturitylength.resize(1, keeper);
    maturitylength[maturitylength.Size() - 1].read(infile, TimeInfo, keeper);
  }

  if (maturitylength.Size() != maturitystep.Size())
    handle.Message("Number of maturitysteps does not equal number of maturitylengths");
  keeper->ClearLast();

  infile >> ws;
  if (!infile.eof()) {
    infile >> text >> ws;
    handle.Unexpected("<end of file>", text);
  }
}

void MaturityB::Print(ofstream& outfile) const {
  int i;
  Maturity::Print(outfile);
  outfile << "\tMaturity timesteps";
  for (i = 0; i < maturitystep.Size(); i++)
    outfile << sep << maturitystep[i];
  outfile << "\n\tMaturity lengths";
  for (i = 0; i < maturitylength.Size(); i++)
    outfile << sep << maturitylength[i];
  outfile << endl;
}

void MaturityB::setStock(StockPtrVector& stockvec) {
  this->Maturity::setStock(stockvec);
}

void MaturityB::Precalc(const TimeClass* const TimeInfo) {
  this->Maturity::Precalc(TimeInfo);
  maturitylength.Update(TimeInfo);
}

double MaturityB::MaturationProbability(int age, int length, int growth,
  const TimeClass* const TimeInfo, int area, double weight) {

  int i;
  for (i = 0; i < maturitylength.Size(); i++)
    if (TimeInfo->CurrentStep() == maturitystep[i])
      return (LgrpDiv->Meanlength(length) >= maturitylength[i]);
  return 0.0;
}

int MaturityB::IsMaturationStep(int area, const TimeClass* const TimeInfo) {
  int i;
  for (i = 0; i < maturitystep.Size(); i++)
    if (maturitystep[i] == TimeInfo->CurrentStep())
      return 1;
  return 0;
}

// ********************************************************
// Functions for MaturityC
// ********************************************************
MaturityC::MaturityC(CommentStream& infile, const TimeClass* const TimeInfo,
  Keeper* const keeper, int minage, const IntVector& minabslength, const IntVector& size,
  const IntVector& tmpareas, const LengthGroupDivision* const lgrpdiv, int NoMatconst)
  : MaturityA(infile, TimeInfo, keeper, minage, minabslength, size, tmpareas, lgrpdiv, NoMatconst) {

  ErrorHandler handle;
  char text[MaxStrLength];
  strncpy(text, "", MaxStrLength);

  infile >> text >> ws;
  if (strcasecmp(text, "maturitystep") != 0)
    handle.Unexpected("maturitystep", text);

  int i = 0;
  while (isdigit(infile.peek()) && !infile.eof()) {
    maturitystep.resize(1);
    infile >> maturitystep[i] >> ws;
    i++;
  }

  for (i = 0; i < maturitystep.Size(); i++)
    if (maturitystep[i] < 1 || maturitystep[i] > TimeInfo->StepsInYear())
      handle.Message("Illegal maturity step in constant maturation function");

  if (!infile.eof()) {
    infile >> text >> ws;
    handle.Unexpected("<end of file>", text);
  }
}

void MaturityC::setStock(StockPtrVector& stockvec) {
  this->Maturity::setStock(stockvec);

  int i;
  minMatureAge = 9999;
  double minlength = 9999.0;
  for (i = 0; i < MatureStocks.Size(); i++) {
    if (MatureStocks[i]->Minage() < minMatureAge)
      minMatureAge = MatureStocks[i]->Minage();
    if (MatureStocks[i]->returnLengthGroupDiv()->minLength() < minlength)
      minlength = MatureStocks[i]->returnLengthGroupDiv()->minLength();
  }
  minMatureLength = LgrpDiv->NoLengthGroup(minlength);
}

void MaturityC::Precalc(const TimeClass* const TimeInfo) {
  this->Maturity::Precalc(TimeInfo);
  maturityParameters.Update(TimeInfo);
  double my;
  int age, len;

  if (maturityParameters.DidChange(TimeInfo)) {
    for (age = PrecalcMaturation.Minage(); age <= PrecalcMaturation.Maxage(); age++) {
      for (len = PrecalcMaturation.Minlength(age); len < PrecalcMaturation.Maxlength(age); len++) {
        if ((age >= minMatureAge) && (len >= minMatureLength)) {
          my = exp(-4.0 * maturityParameters[0] * (LgrpDiv->Meanlength(len) - maturityParameters[1])
                 - 4.0 * maturityParameters[2] * (age - maturityParameters[3]));
          PrecalcMaturation[age][len] = 1 / (1 + my);
        } else
          PrecalcMaturation[age][len] = 0.0;
      }
    }
  }
}

double MaturityC::MaturationProbability(int age, int length, int growth,
  const TimeClass* const TimeInfo, int area, double weight) {

  if (this->IsMaturationStep(area, TimeInfo))
    return (min(max(0.0, PrecalcMaturation[age][length]), 1.0));
  return 0.0;
}

int MaturityC::IsMaturationStep(int area, const TimeClass* const TimeInfo) {
  int i;
  for (i = 0; i < maturitystep.Size(); i++)
    if (maturitystep[i] == TimeInfo->CurrentStep())
      return 1;
  return 0;
}

// ********************************************************
// Functions for MaturityD
// ********************************************************
MaturityD::MaturityD(CommentStream& infile, const TimeClass* const TimeInfo,
  Keeper* const keeper, int minage, const IntVector& minabslength, const IntVector& size,
  const IntVector& tmpareas, const LengthGroupDivision* const lgrpdiv, int NoMatconst, const char* refWeightFile)
  : MaturityA(infile, TimeInfo, keeper, minage, minabslength, size, tmpareas, lgrpdiv, NoMatconst) {

  ErrorHandler handle;
  char text[MaxStrLength];
  strncpy(text, "", MaxStrLength);

  infile >> text >> ws;
  if (strcasecmp(text, "maturitystep") != 0)
    handle.Unexpected("maturitystep", text);

  int i = 0;
  while (isdigit(infile.peek()) && !infile.eof()) {
    maturitystep.resize(1);
    infile >> maturitystep[i] >> ws;
    i++;
  }

  for (i = 0; i < maturitystep.Size(); i++)
    if (maturitystep[i] < 1 || maturitystep[i] > TimeInfo->StepsInYear())
      handle.Message("Illegal maturity step in constantweight maturation function");

  if (!infile.eof()) {
    infile >> text >> ws;
    handle.Unexpected("<end of file>", text);
  }

  //read information on reference weights.
  ifstream subweightfile;
  subweightfile.open(refWeightFile, ios::in);
  checkIfFailure(subweightfile, refWeightFile);
  handle.Open(refWeightFile);
  CommentStream subweightcomment(subweightfile);
  DoubleMatrix tmpRefW;
  if (!read2ColVector(subweightcomment, tmpRefW))
    handle.Message("Wrong format for reference weights");
  handle.Close();
  subweightfile.close();
  subweightfile.clear();

  //Aggregate the reference weight data to be the same length groups
  refWeight.resize(LgrpDiv->NoLengthGroups(), 0.0);
  int j, pos = 0;
  double tmp;
  for (j = 0; j < LgrpDiv->NoLengthGroups(); j++) {
    for (i = pos; i < tmpRefW.Nrow() - 1; i++) {
      if (LgrpDiv->Meanlength(j) >= tmpRefW[i][0] && LgrpDiv->Meanlength(j) <= tmpRefW[i + 1][0]) {
        tmp = (LgrpDiv->Meanlength(j) - tmpRefW[i][0]) / (tmpRefW[i + 1][0] - tmpRefW[i][0]);
        refWeight[j] = tmpRefW[i][1] + tmp * (tmpRefW[i + 1][1] - tmpRefW[i][1]);
        pos = i;
      }
    }
  }
}

void MaturityD::setStock(StockPtrVector& stockvec) {
  this->Maturity::setStock(stockvec);

  int i;
  minMatureAge = 9999;
  double minlength = 9999.0;
  for (i = 0; i < MatureStocks.Size(); i++) {
    if (MatureStocks[i]->Minage() < minMatureAge)
      minMatureAge = MatureStocks[i]->Minage();
    if (MatureStocks[i]->returnLengthGroupDiv()->minLength() < minlength)
      minlength = MatureStocks[i]->returnLengthGroupDiv()->minLength();
  }
  minMatureLength = LgrpDiv->NoLengthGroup(minlength);
}

void MaturityD::Precalc(const TimeClass* const TimeInfo) {
  this->Maturity::Precalc(TimeInfo);
  maturityParameters.Update(TimeInfo);
}

double MaturityD::MaturationProbability(int age, int length, int growth,
  const TimeClass* const TimeInfo, int area, double weight) {

  if ((this->IsMaturationStep(area, TimeInfo)) && (age >= minMatureAge) && (length >= minMatureLength)) {
    double tmpweight, my, ratio;

    if (length >= refWeight.Size())
      tmpweight = maturityParameters[5];
    else if (isZero(refWeight[length]))
      tmpweight = maturityParameters[5];
    else
      tmpweight = weight / refWeight[length];

    my = exp(-4.0 * maturityParameters[0] * (LgrpDiv->Meanlength(length) - maturityParameters[1])
           - 4.0 * maturityParameters[2] * (age - maturityParameters[3])
           - 4.0 * maturityParameters[4] * (tmpweight - maturityParameters[5]));
    ratio = 1 / (1 + my);
    return (min(max(0.0, ratio), 1.0));
  }
  return 0.0;
}

int MaturityD::IsMaturationStep(int area, const TimeClass* const TimeInfo) {
  int i;
  for (i = 0; i < maturitystep.Size(); i++)
    if (maturitystep[i] == TimeInfo->CurrentStep())
      return 1;
  return 0;
}
