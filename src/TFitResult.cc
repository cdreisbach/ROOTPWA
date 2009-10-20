///////////////////////////////////////////////////////////////////////////
//
//    Copyright 2009 Sebastian Neubert
//
//    This file is part of rootpwa
//
//    rootpwa is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    rootpwa is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with rootpwa.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////
//-----------------------------------------------------------
// File and Version Information:
// $Id$
//
// Description:
//      Data storage class for PWA fit result of one kinematic bin
//
// Environment:
//      Software developed for the COMPASS experiment at CERN
//
// Author List:
//      Sebastian Neubert    TUM            (original author)
//
//
//-----------------------------------------------------------


#include <algorithm>

// PWA2000 classes
#include "integral.h"

#include "TFitResult.h"


using namespace std;


ClassImp(TFitResult);


TFitResult::TFitResult()
  : _nevt         (0),
    _nmbEvents    (0),
    _massBinCenter(0),
    _logLikelihood(0),
    _rank         (0),
    _hasErrors    (false)
{ }


TFitResult::TFitResult(const TFitBin& fitBin)
  : _nevt                  (fitBin.normNmbEvents()),
    _nmbEvents             (fitBin.nmbEvents()),
    _massBinCenter         (fitBin.massBinCenter()),
    _logLikelihood         (fitBin.logLikelihood()),
    _rank                  (fitBin.rank()),
    _hasErrors             (fitBin.fitParCovMatrixValid()),
    _fitParCovMatrix       (fitBin.fitParCovMatrix()),
    _fitParCovMatrixIndices(fitBin.fitParCovIndices()),
    _normIntegral          (fitBin.normIntegral()),
    _normIntIndexMap       (fitBin.prodAmpIndexMap())
{
  {
    const unsigned int nmbAmps = fitBin.prodAmps().size();
    _prodAmps.resize(nmbAmps, 0);
    for (unsigned int i = 0; i < nmbAmps; ++i)
      _prodAmps[i] = complex<double>(fitBin.prodAmps()[i].Re(), fitBin.prodAmps()[i].Im());
  }
  {
    const unsigned int nmbAmpNames = fitBin.prodAmpNames().size();
    _prodAmpNames.resize(nmbAmpNames, "");
    for (unsigned int i = 0; i < nmbAmpNames; ++i)
      _prodAmpNames[i] = fitBin.prodAmpNames()[i].Data();
  }
  {
    const unsigned int nmbWaveNames = fitBin.waveNames().size();
    _waveNames.resize(nmbWaveNames, "");
    for (unsigned int i = 0; i < nmbWaveNames; ++i)
      _waveNames[i] = fitBin.waveNames()[i].Data();
  }
}


TFitResult::~TFitResult()
{ }


/// \brief calculates spin density matrix element for waves A and B
///
/// \f[ \rho_{AB} = \sum_r V_{Ar} V_{Br}^* \f]
complex<double>
TFitResult::spinDensityMatrixElem(const unsigned int waveIndexA,
				  const unsigned int waveIndexB) const
{
  // get pairs of amplitude indices with the same rank for waves A and B
  const vector<pair<unsigned int, unsigned int> > prodAmpIndexPairs = prodAmpIndexPairsForWaves(waveIndexA, waveIndexB);
  if (prodAmpIndexPairs.size() == 0)
    return 0;
  // sum up amplitude products
  complex<double> spinDens = 0;
  for (unsigned int i = 0; i < prodAmpIndexPairs.size(); ++i)
    spinDens += _prodAmps[prodAmpIndexPairs[i].first] * conj(_prodAmps[prodAmpIndexPairs[i].second]);
  return spinDens;
}


/// returns fit parameter value by parameter name
double
TFitResult::fitParameter(const string& parName) const
{
  // check if parameter corresponds to real or imaginary part of production amplitude
  TString name(parName);
  bool    realPart = false;
  if (name.Contains("flat")) {
    name     = "flat";
    realPart = true;
  } else
    realPart = name.Contains("RE");
  // find corresponding production amplitude
  if (realPart)
    name.ReplaceAll("_RE", "");
  else
    name.ReplaceAll("_IM", "");
  for (unsigned int i = 0; i < nmbProdAmps(); ++i)
    if (name == _prodAmpNames[i]) {
      if (realPart)
	return _prodAmps[i].real();
      else
	return _prodAmps[i].imag();
    }
  // not found
  printWarn << "Could not find any parameter named '" << parName << "'." << endl;
  return 0;
}


/// \brief constructs 2n x 2n covariance matrix of production amplitudes specified by index list
// where n is the number of amplitudes
// layout:
//         cov(A0.re, A0.re)        cov(A0.re, A0.im)        ...  cov(A0.re, A(n - 1).re)        cov(A0.re, A(n - 1).im)
//         cov(A0.im, A0.re)        cov(A0.im, A0.im)        ...  cov(A0.im, A(n - 1).re)        cov(A0.im, A(n - 1).im)
//                  .                        .                             .                              .
//                  .                        .                             .                              .
//                  .                        .                             .                              .
//         cov(A(n - 1).re, A0.re)  cov(A(n - 1).re, A0.im)  ...  cov(A(n - 1).re, A(n - 1).re)  cov(A(n - 1).re, A(n - 1).im)
//         cov(A(n - 1).im, A0.re)  cov(A(n - 1).im, A0.im)  ...  cov(A(n - 1).im, A(n - 1).re)  cov(A(n - 1).im, A(n - 1).im)
// !!! possible optimization: exploit symmetry of cov matrix
TMatrixT<double>
TFitResult::prodAmpCov(const vector<unsigned int>& prodAmpIndices) const
{
  const unsigned int dim = 2 * prodAmpIndices.size();
  TMatrixT<double>   prodAmpCov(dim, dim);
  if (!_hasErrors) {
    printWarn << "TFitResult does not have a valid error matrix. Returning zero covariance matrix." << endl;
    return prodAmpCov;
  }
  // get corresponding indices for parameter covariances
  vector<int> parCovIndices;
  for (unsigned int i = 0; i < prodAmpIndices.size(); ++i) {
    parCovIndices.push_back(_fitParCovMatrixIndices[prodAmpIndices[i]].first);   // real part
    parCovIndices.push_back(_fitParCovMatrixIndices[prodAmpIndices[i]].second);  // imaginary part
  }
  // build covariance matrix
  for (unsigned int row = 0; row < dim; ++row)
    for (unsigned int col = 0; col < dim; ++col) {
      const int i = parCovIndices[row];
      const int j = parCovIndices[col];
      if ((i >= 0) && (j >= 0))
	prodAmpCov[row][col] = fitParameterCov(i, j);
    }
  return prodAmpCov;
}


/// \brief calculates covariance matrix of spin density matrix element for waves A and B
///
/// rho_AB = sum_r V_Ar V_Br^*
// !!! possible optimization: make special case for waveIndexA == waveIndexB
TMatrixT<double>
TFitResult::spinDensityMatrixElemCov(const unsigned int waveIndexA,
				     const unsigned int waveIndexB) const
{
  // get pairs of amplitude indices with the same rank for waves A and B
  const vector<pair<unsigned int, unsigned int> > prodAmpIndexPairs = prodAmpIndexPairsForWaves(waveIndexA, waveIndexB);
  if (!_hasErrors || (prodAmpIndexPairs.size() == 0)) {
    TMatrixT<double> spinDensCov(2, 2);
    return spinDensCov;
  }
  // build covariance matrix for amplitudes
  const TMatrixT<double> prodAmpCov = this->prodAmpCov(prodAmpIndexPairs);
  // build Jacobian for rho_AB, which is a 2 x 4m matrix composed of 2m sub-Jacobians:
  // J = (JA0, ..., JA(m - 1), JB0, ..., JB(m - 1))
  // m is the number of production amplitudes for waves A and B that have the same rank
  const unsigned int dim = prodAmpCov.GetNcols();
  TMatrixT<double>   jacobian(2, dim);
  // build m sub-Jacobians for d rho_AB / d V_Ar = M(V_Br^*)
  for (unsigned int i = 0; i < prodAmpIndexPairs.size(); ++i) {
    const unsigned int     ampIndexB    = prodAmpIndexPairs[i].second;
    const TMatrixT<double> subJacobianA = matrixRepr(conj(_prodAmps[ampIndexB]));
    jacobian.SetSub(0, 2 * i, subJacobianA);
  }
  // build m sub-Jacobian for d rho_AB / d V_Br = M(V_Ar) {{1,  0},
  //                                                       {0, -1}}
  TMatrixT<double> M(2, 2);  // complex conjugation of V_Br is non-analytic operation
  M[0][0] =  1;
  M[1][1] = -1;
  const unsigned int colOffset = 2 * prodAmpIndexPairs.size();
  for (unsigned int i = 0; i < prodAmpIndexPairs.size(); ++i) {
    const unsigned int ampIndexA    = prodAmpIndexPairs[i].first;
    TMatrixT<double>   subJacobianB = matrixRepr(_prodAmps[ampIndexA]);
    subJacobianB *= M;
    jacobian.SetSub(0, colOffset + 2 * i, subJacobianB);
  }
  // calculate spin density covariance matrix cov(rho_AB) = J cov(V_A0, ..., V_A(m - 1), V_B0, ..., V_B(m - 1)) J^T
  const TMatrixT<double> jacobianT(TMatrixT<double>::kTransposed, jacobian);
  // binary operations are unavoidable, since matrices are not squared
  // !!! possible optimaztion: use special TMatrixT constructors to perform the multiplication
  const TMatrixT<double> prodAmpCovJT = prodAmpCov * jacobianT;
  const TMatrixT<double> spinDensCov  = jacobian   * prodAmpCovJT;
  return spinDensCov;
}


/// \brief calculates intensity for set of waves matching name pattern
///
/// int = sum_i int(i) + sum_i sum_{j < i} overlap(i, j)
double
TFitResult::intensity(const char* waveNamePattern) const
{
  vector<unsigned int> waveIndices = waveIndicesMatchingPattern(waveNamePattern);
  double intensity = 0;
  for (unsigned int i = 0; i < waveIndices.size(); ++i) {
    intensity += this->intensity(waveIndices[i]);
    for (unsigned int j = 0; j < i; ++j)
      intensity += overlap(waveIndices[i], waveIndices[j]);
  }
  return intensity;
}


/// finds wave indices for production amplitues A and B and returns the normalization integral of the two waves
complex<double>
TFitResult::normIntegralForProdAmp(const unsigned int prodAmpIndexA,
				   const unsigned int prodAmpIndexB) const
{
  // treat special case of flat wave which has no normalization integral
  const bool flatWaveA = prodAmpName(prodAmpIndexA).Contains("flat");
  const bool flatWaveB = prodAmpName(prodAmpIndexB).Contains("flat");
  if (flatWaveA && flatWaveB)
    return 1;
  else if (flatWaveA || flatWaveB)
    return 0;
  else {
    map<int, int>::const_iterator indexA = _normIntIndexMap.find(prodAmpIndexA);
    map<int, int>::const_iterator indexB = _normIntIndexMap.find(prodAmpIndexB);
    if ((indexA == _normIntIndexMap.end()) || (indexB == _normIntIndexMap.end())) {
      printWarn << "Amplitude index " << prodAmpIndexA << " or " << prodAmpIndexB << " is out of bound." << endl;
      return 0;
    }
    return normIntegral(indexA->second, indexB->second);
  }
}


/// \brief calculates error of intensity of a set of waves matching name pattern
///
/// error calculation is performed on amplitude level using: int = sum_ij Norm_ij sum_r A_ir A_jr*
double 
TFitResult::intensityErr(const char* waveNamePattern) const
{
  // get amplitudes that correspond to wave name pattern
  const vector<unsigned int> prodAmpIndices = prodAmpIndicesMatchingPattern(waveNamePattern);
  const unsigned int         nmbAmps        = prodAmpIndices.size();
  if (!_hasErrors || (nmbAmps == 0))
    return 0;
  // build Jacobian for intensity, which is a 1 x 2n matrix composed of n sub-Jacobians:
  // J = (JA_0, ..., JA_{n - 1}), where n is the number of production amplitudes
  TMatrixT<double> jacobian(1, 2 * nmbAmps);
  for (unsigned int i = 0; i < nmbAmps; ++i) {
    // build sub-Jacobian for each amplitude; intensity is real valued function, so J has only one row
    // JA_ir = 2 * sum_j (A_jr Norm_ji)
    complex<double>  ampNorm     = 0;  // sum_j (A_jr Norm_ji)
    const int        currentRank = rankOfProdAmp(prodAmpIndices[i]);
    for (unsigned int j = 0; j < nmbAmps; ++j) {
      if (rankOfProdAmp(prodAmpIndices[j]) != currentRank)
	continue;
      ampNorm += _prodAmps[prodAmpIndices[j]] * normIntegralForProdAmp(j, i);  // order of indices is essential
    }
    jacobian[0][2 * i    ] = ampNorm.real();
    jacobian[0][2 * i + 1] = ampNorm.imag();
  }
  jacobian *= 2;
  const TMatrixT<double> prodAmpCov   = this->prodAmpCov(prodAmpIndices);     // 2n x 2n matrix
  const TMatrixT<double> jacobianT(TMatrixT<double>::kTransposed, jacobian);  // 2n x  1 matrix
  const TMatrixT<double> prodAmpCovJT = prodAmpCov * jacobianT;               // 2n x  1 matrix
  const TMatrixT<double> intensityCov = jacobian * prodAmpCovJT;              //  1 x  1 matrix
  return sqrt(intensityCov[0][0]);
}


/// calculates phase difference between wave A and wave B
double
TFitResult::phase(const unsigned int waveIndexA,
		  const unsigned int waveIndexB) const
{ 
  if (waveIndexA == waveIndexB)
    return 0;
  return arg(spinDensityMatrixElem(waveIndexA, waveIndexB)) * TMath::RadToDeg();
}


/// calculates error of phase difference between wave A and wave B
double
TFitResult::phaseErr(const unsigned int waveIndexA,
		     const unsigned int waveIndexB) const
{
  if ((!_hasErrors) || (waveIndexA == waveIndexB))
    return 0;
  // construct Jacobian for phi_AB = +- arctan(Im[rho_AB] / Re[rho_AB])
  const complex<double> spinDens = spinDensityMatrixElem(waveIndexA, waveIndexB);
  TMatrixT<double>      jacobian(1, 2);  // phase is real valued function, so J has only one row
  {
    const double x = spinDens.real();
    const double y = spinDens.imag();
    if ((x != 0) || (y != 0)) {
      jacobian[0][0] = 1 / (x + y * y / x);
      jacobian[0][1] = -y / (x * x + y * y);
    }
  }
  // calculate variance
  const double phaseVariance = realValVariance(waveIndexA, waveIndexB, jacobian);
  return sqrt(phaseVariance) * TMath::RadToDeg();
}


/// calculates coherence of wave A and wave B
double
TFitResult::coherence(const unsigned int waveIndexA,
		      const unsigned int waveIndexB) const
{
  const double          rhoAA = spinDensityMatrixElem(waveIndexA, waveIndexA).real();  // rho_AA is real by definition
  const double          rhoBB = spinDensityMatrixElem(waveIndexB, waveIndexB).real();  // rho_BB is real by definition
  const complex<double> rhoAB = spinDensityMatrixElem(waveIndexA, waveIndexB);
  return sqrt(std::norm(rhoAB) / (rhoAA * rhoBB));
}


/// calculates error of coherence of wave A and wave B
double
TFitResult::coherenceErr(const unsigned int waveIndexA,
			 const unsigned int waveIndexB) const
{
  // get amplitude indices for waves A and B
  const vector<unsigned int> prodAmpIndices[2] = {prodAmpIndicesForWave(waveIndexA),
						  prodAmpIndicesForWave(waveIndexB)};
  if (!_hasErrors || (prodAmpIndices[0].size() == 0) || (prodAmpIndices[1].size() == 0))
    return 0;
  // build Jacobian for coherence, which is a 1 x 2(n + m) matrix composed of (n + m) sub-Jacobians:
  // J = (JA_0, ..., JA_{n - 1}, JB_0, ..., JB_{m - 1})
  const unsigned int nmbAmps = prodAmpIndices[0].size() + prodAmpIndices[1].size();
  TMatrixT<double>   jacobian(1, 2 * nmbAmps);
  // precalculate some variables
  const double          rhoAA     = spinDensityMatrixElem(waveIndexA, waveIndexA).real();  // rho_AA is real by definition
  const double          rhoBB     = spinDensityMatrixElem(waveIndexB, waveIndexB).real();  // rho_BB is real by definition
  const complex<double> rhoAB     = spinDensityMatrixElem(waveIndexA, waveIndexB);
  const double          rhoABRe   = rhoAB.real();
  const double          rhoABIm   = rhoAB.imag();
  const double          rhoABNorm = std::norm(rhoAB);
  const double          coh       = sqrt(rhoABNorm / (rhoAA * rhoBB));
  if (coh == 0)
    return 0;
  // build m sub-Jacobians for JA_r = coh_AB / d V_Ar
  for (unsigned int i = 0; i < prodAmpIndices[0].size(); ++i) {
    const unsigned int    prodAmpIndexA = prodAmpIndices[0][i];
    const complex<double> prodAmpA      = _prodAmps[prodAmpIndexA];
    const int             prodAmpRankA  = rankOfProdAmp(prodAmpIndexA);
    // find production amplitude of wave B with same rank
    complex<double> prodAmpB = 0;
    for (unsigned int j = 0; j < prodAmpIndices[1].size(); ++j) {
      const unsigned int prodAmpIndexB = prodAmpIndices[1][j];
      if (rankOfProdAmp(prodAmpIndexB) == prodAmpRankA) {
	prodAmpB = _prodAmps[prodAmpIndexB];
	break;
      }
    }
    jacobian[0][2 * i    ] = rhoABRe * prodAmpB.real() - rhoABIm * prodAmpB.imag() - (rhoABNorm / rhoAA) * prodAmpA.real();
    jacobian[0][2 * i + 1] = rhoABRe * prodAmpB.imag() + rhoABIm * prodAmpB.real() - (rhoABNorm / rhoAA) * prodAmpA.imag();
  }
// !!! possible optimization: join the loops for JA_r and JB_r
  // build m sub-Jacobian for JB_r = d coh_AB / d V_Br
  const unsigned int colOffset = 2 * prodAmpIndices[0].size();
  for (unsigned int i = 0; i < prodAmpIndices[1].size(); ++i) {
    const unsigned int    prodAmpIndexB = prodAmpIndices[1][i];
    const complex<double> prodAmpB      = _prodAmps[prodAmpIndexB];
    const int             prodAmpRankB  = rankOfProdAmp(prodAmpIndexB);
    // find production amplitude of wave A with same rank
    complex<double> prodAmpA = 0;
    for (unsigned int j = 0; j < prodAmpIndices[0].size(); ++j) {
      const unsigned int prodAmpIndexA = prodAmpIndices[0][j];
      if (rankOfProdAmp(prodAmpIndexA) == prodAmpRankB) {
	prodAmpA = _prodAmps[prodAmpIndexA];
	break;
      }
    }
    jacobian[0][colOffset + 2 * i    ] = rhoABRe * prodAmpA.real() + rhoABIm * prodAmpA.imag() - (rhoABNorm / rhoBB) * prodAmpB.real();
    jacobian[0][colOffset + 2 * i + 1] = rhoABRe * prodAmpA.imag() - rhoABIm * prodAmpA.real() - (rhoABNorm / rhoBB) * prodAmpB.imag();
  }
  jacobian *= 1 / (coh * rhoAA * rhoBB);
  // build covariance matrix for amplitudes and calculate coherence covariance matrix
  const TMatrixT<double> prodAmpCov   = this->prodAmpCov(prodAmpIndices[0], prodAmpIndices[1]);  // 2(n + m) x 2(n + m) matrix
  const TMatrixT<double> jacobianT(TMatrixT<double>::kTransposed, jacobian);                     // 2(n + m) x        1 matrix
  const TMatrixT<double> prodAmpCovJT = prodAmpCov * jacobianT;                                  // 2(n + m) x        1 matrix
  const TMatrixT<double> cohCov       = jacobian   * prodAmpCovJT;                               //        1 x        1 matrix
  return sqrt(cohCov[0][0]);
}


/// calculates overlap of wave A and wave B
double
TFitResult::overlap(const unsigned int waveIndexA,
		    const unsigned int waveIndexB) const
{
  const complex<double> spinDens = spinDensityMatrixElem(waveIndexA, waveIndexB);
  const complex<double> normInt  = normIntegral         (waveIndexA, waveIndexB);
  return 2 * (spinDens * normInt).real();
}


/// calculates error of overlap of wave A and wave B
double
TFitResult::overlapErr(const unsigned int waveIndexA,
		       const unsigned int waveIndexB) const
{
  if (!_hasErrors)
    return 0;
  const complex<double> normInt = normIntegral(waveIndexA, waveIndexB);
  TMatrixT<double> jacobian(1, 2);  // overlap is real valued function, so J has only one row
  jacobian[0][0] =  2 * normInt.real();
  jacobian[0][1] = -2 * normInt.imag();
  const double overlapVariance = realValVariance(waveIndexA, waveIndexB, jacobian);
  return sqrt(overlapVariance);
}


void
TFitResult::reset()
{
  _nevt          = 0;
  _nmbEvents     = 0;
  _massBinCenter = 0;
  _logLikelihood = 0;
  _rank          = 0;
  _prodAmps.clear();
  _prodAmpNames.clear();
  _waveNames.clear();
  _hasErrors = false;
  _fitParCovMatrix.ResizeTo(0, 0);
  _fitParCovMatrixIndices.clear();
  _normIntegral.ResizeTo(0, 0);
  _normIntIndexMap.clear();
}


void 
TFitResult::fill(const std::vector<TComplex>&                 prodAmps,
		 const std::vector<std::pair<Int_t, Int_t> >& fitParCovMatrixIndices,
		 const std::vector<TString>&                  prodAmpNames,
		 const Int_t                                  nevt,
		 const UInt_t                                 nmbEvents,
		 const Double_t                               massBinCenter,
		 const TCMatrix&                              normIntegral,
		 const TMatrixT<Double_t>&                    fitParCovMatrix,
		 const Double_t                               logLikelihood,
		 const Int_t                                  rank)
{
// !!! add some consistency checks
  _nevt          = nevt;
  _nmbEvents     = nmbEvents;
  _massBinCenter = massBinCenter;
  _logLikelihood = logLikelihood;
  _rank          = rank;
  {
    const unsigned int nmbAmps = prodAmps.size();
    _prodAmps.resize(nmbAmps, 0);
    for (unsigned int i = 0; i < nmbAmps; ++i)
      _prodAmps[i] = complex<double>(prodAmps[i].Re(), prodAmps[i].Im());
  }
  {
    const unsigned int nmbAmpNames = prodAmpNames.size();
    _prodAmpNames.resize(nmbAmpNames, "");
    for (unsigned int i = 0; i < nmbAmpNames; ++i)
      _prodAmpNames[i] = prodAmpNames[i].Data();
  }
  _fitParCovMatrix.ResizeTo(fitParCovMatrix.GetNrows(), fitParCovMatrix.GetNcols());
  _fitParCovMatrix        = fitParCovMatrix;
  _fitParCovMatrixIndices = fitParCovMatrixIndices;
  if ((fitParCovMatrix.GetNrows() == 0) || (fitParCovMatrix.GetNcols() == 0))  // check whether there really is an error matrix
    _hasErrors = false;
  _normIntegral.ResizeTo(normIntegral.nrows(), normIntegral.ncols());
  _normIntegral = normIntegral;

  buildWaveMap();
}


void
TFitResult::buildWaveMap() {
  int n=_prodAmpNames.size();
  for(int i=0;i<n;++i){
    // strip rank
    TString title=wavetitle(i);
    cout << "title=="<<title<<endl;
    if(find(_waveNames.begin(),_waveNames.end(),title.Data())==_waveNames.end()){
      cout<<"Wave: "<<title<<endl;
      _waveNames.push_back(title.Data());
    }
    
    // look for index of first occurence
    int j;
    for(j=0;j<n;++j)
      if(prodAmpName(j).Contains(title))
	break;
    _normIntIndexMap[i]=j;
  }
}