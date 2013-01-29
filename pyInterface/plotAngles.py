#!/usr/bin/python2.7

import argparse
import glob
import sys
import time

import pyRootPwa
import pyRootPwa.utils


if __name__ == "__main__":

	# parse command line arguments
	parser = argparse.ArgumentParser(
	                                 description="No help text available."
	                                )
	parser.add_argument("outputFile", metavar="output-file", help="path to output file")
	parser.add_argument("templateFile", metavar="template-file", help="path to template file")
	parser.add_argument("-b", action="append", metavar="massBin(s)", default=[], dest="massBins", help="mass bins to be calculated (default: all)")
	parser.add_argument("-c", type=str, metavar="config-file", default="rootpwa.config", dest="configFileName", help="path to config file (default: ./rootpwa.config)")
	arguments = parser.parse_args()
	if len(arguments.massBins) == 0:
		arguments.massBins.append("all")

	pyRootPwa.utils.stdoutisatty = sys.stdout.isatty()
	pyRootPwa.utils.stderrisatty = sys.stderr.isatty()

	printingCounter = 5 * [0]
	pyRootPwa.utils.printErr = pyRootPwa.utils.printErrClass(printingCounter)
	pyRootPwa.utils.printWarn = pyRootPwa.utils.printWarnClass(printingCounter)
	pyRootPwa.utils.printSucc = pyRootPwa.utils.printSuccClass(printingCounter)
	pyRootPwa.utils.printInfo = pyRootPwa.utils.printInfoClass(printingCounter)
	pyRootPwa.utils.printDebug = pyRootPwa.utils.printDebugClass(printingCounter)

	pyRootPwa.config = pyRootPwa.rootPwaConfig(arguments.configFileName)
	pyRootPwa.core.particleDataTable.readFile(pyRootPwa.config.pdgFileName)

	waveDesc = pyRootPwa.core.waveDescription()
	waveDesc.parseKeyFile(arguments.templateFile)
	(result, topology) = waveDesc.constructDecayTopology(True)

	if not result:
		pyRootPwa.utils.printErr("could not construct topology. Aborting...")
		sys.exit(5)

	permutations = {}
	boseSyms = topology.getBoseSymmetrization()
	for sym in boseSyms:
		permutation = tuple(sym["fsPartPermMap"])
		permutations[permutation] = []
		if list(permutation) == [x for x in range(topology.nmbFsParticles())]:
			for i in range(topology.nmbDecayVertices()):
				permutations[permutation].append((True, True))
		else:
			for vertex in topology.isobarDecayVertices():
				permutations[permutation].append((topology.isobarIsAffectedByPermutation(vertex, list(permutation)),
				                                  topology.daughtersAreAffectedByPermutation(vertex, list(permutation))))


	outputFile = pyRootPwa.ROOT.TFile.Open(arguments.outputFile, "RECREATE")

	inputFileRanges = {}
	hists = {}
	for binRange in arguments.massBins:
		allMassBins = sorted(glob.glob(pyRootPwa.config.dataDirectory + '/' + pyRootPwa.config.massBinDirectoryNamePattern))
		massBins = pyRootPwa.utils.parseMassBinArgs(allMassBins, binRange)
		rangeName = massBins[0].rsplit('/', 1)[-1] + '_' + massBins[-1].rsplit('/', 1)[-1]
		inputFileRanges[rangeName] = pyRootPwa.utils.getListOfInputFiles(massBins)[0]
		outputFile.mkdir(rangeName)
		outputFile.cd(rangeName)
		hists[rangeName] = []
		for i in range(topology.nmbDecayVertices()):
			hists[rangeName].append([pyRootPwa.ROOT.TH1D("m" + str(i), "m" + str(i), 100, 0, 5),
									 pyRootPwa.ROOT.TH1D("phi" + str(i), "phi" + str(i), 100, -3.142, 3.142),
			                         pyRootPwa.ROOT.TH1D("theta" + str(i), "theta" + str(i), 100, -1, 1)])
	assert(inputFileRanges.keys() == hists.keys())

	for rangeName in inputFileRanges.keys():

		pyRootPwa.utils.printInfo("Processing bin range " + rangeName)
		outputFile.cd(rangeName)

		for dataFileName in inputFileRanges[rangeName]:

			# Do all the initialization
			pyRootPwa.utils.printInfo("Opening input file " + dataFileName)
			dataFile = pyRootPwa.ROOT.TFile.Open(dataFileName)
			dataTree = dataFile.Get(pyRootPwa.config.inTreeName)

			prodKinParticles = dataFile.Get(pyRootPwa.config.prodKinPartNamesObjName)
			decayKinParticles = dataFile.Get(pyRootPwa.config.decayKinPartNamesObjName)
			topology.initKinematicsData(prodKinParticles, decayKinParticles)
			nEvents = dataTree.GetEntries()
			prodKinMomenta = pyRootPwa.ROOT.TClonesArray("TVector3")
			decayKinMomenta = pyRootPwa.ROOT.TClonesArray("TVector3")
			dataTree.SetBranchAddress(pyRootPwa.config.prodKinMomentaLeafName, prodKinMomenta)
			dataTree.SetBranchAddress(pyRootPwa.config.decayKinMomentaLeafName, decayKinMomenta)

			progressbar = pyRootPwa.utils.progressBar(0, nEvents-1, sys.stdout)
			progressbar.start()

			# Loop over Events
			for i in range(nEvents):
				dataTree.GetEntry(i)

				# Read input data
				topology.readKinematicsData(prodKinMomenta, decayKinMomenta)

				for permutationKey in permutations.keys():

					permutation = list(permutationKey)

					topology.revertMomenta(permutation)

					# Transform all particles
					topology.calcIsobarLzVec()
					beamLv = topology.productionVertex().referenceLzVec()
					XLv = topology.XParticle().lzVec
					gjTrans = pyRootPwa.core.isobarAmplitude.gjTransform(beamLv, XLv)
					for vertex in topology.isobarDecayVertices():
						vertex.transformOutParticles(gjTrans)
					for vertex in topology.isobarDecayVertices()[1:]:
						hfTrans = pyRootPwa.core.isobarHelicityAmplitude.hfTransform(vertex.parent().lzVec)
						for subVertex in topology.subDecay(vertex).decayVertices():
							subVertex.transformOutParticles(hfTrans)

					# Fill the histograms
					hist_i = 0

					for vertex in topology.isobarDecayVertices():
						if(permutations[permutationKey][hist_i][0]):
							parent = vertex.parent()
							mass = parent.lzVec.M()
							hists[rangeName][hist_i][0].Fill(mass)
						if(permutations[permutationKey][hist_i][1]):
							daughter = vertex.daughter1()
							phi = daughter.lzVec.Phi()
							theta = daughter.lzVec.Theta()
							hists[rangeName][hist_i][1].Fill(phi)
							hists[rangeName][hist_i][2].Fill(pyRootPwa.ROOT.TMath.Cos(theta))
						hist_i += 1

				progressbar.update(i)

			dataFile.Close()

	outputFile.Write()
	outputFile.Close()

	pyRootPwa.utils.printPrintingSummary(printingCounter)
