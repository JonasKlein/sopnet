#include <imageprocessing/ComponentTree.h>
#include <imageprocessing/Mser.h>
#include <sopnet/features/Overlap.h>
#include <util/ProgramOptions.h>
#include "ComponentTreeConverter.h"
#include "StackSliceExtractor.h"

static logger::LogChannel stacksliceextractorlog("stacksliceextractorlog", "[StackSliceExtractor] ");

util::ProgramOption optionSimilarityThreshold(
		util::_module           = "sopnet.slices",
		util::_long_name        = "similarityThreshold",
		util::_description_text = "The minimum normalized overlap [#overlap/(#size1 + #size2 - #overlap)] for which two slices are considered the same.",
		util::_default_value    = 0.75);

StackSliceExtractor::StackSliceExtractor(unsigned int section) :
	_section(section),
	_sliceImageExtractor(boost::make_shared<ImageExtractor>()),
	_mserParameters(boost::make_shared<MserParameters>()),
	_sliceCollector(boost::make_shared<SliceCollector>()) {

	registerInput(_sliceImageStack, "slices");
	registerInput(_forceExplanation, "force explanation");
	registerOutput(_sliceCollector->getOutput("slices"), "slices");
	registerOutput(_sliceCollector->getOutput("linear constraints"), "linear constraints");

	_sliceImageStack.registerBackwardCallback(&StackSliceExtractor::onInputSet, this);

	// set default mser parameters from program options
	_mserParameters->darkToBright = false;
	_mserParameters->brightToDark = true;
	_mserParameters->minArea      = 0;
	_mserParameters->maxArea      = 100000000;
}

void
StackSliceExtractor::onInputSet(const pipeline::InputSet<ImageStack>& signal) {

	LOG_DEBUG(stacksliceextractorlog) << "image stack set" << std::endl;

	// connect input image stack to slice image extractor
	_sliceImageExtractor->setInput(_sliceImageStack.getAssignedOutput());

	// clear slice collector content
	_sliceCollector->clearInputs(0);

	// for each image in the stack, set up the pipeline
	for (int i = 0; i < _sliceImageStack->size(); i++) {

		boost::shared_ptr<Mser> mser = boost::make_shared<Mser>();

		mser->setInput("image", _sliceImageExtractor->getOutput(i));
		mser->setInput("parameters", _mserParameters);

		boost::shared_ptr<ComponentTreeConverter> converter = boost::make_shared<ComponentTreeConverter>(_section);

		converter->setInput(mser->getOutput());

		_sliceCollector->addInput(converter->getOutput());
	}

	LOG_DEBUG(stacksliceextractorlog) << "internal pipeline set up" << std::endl;
}

StackSliceExtractor::SliceCollector::SliceCollector() {

	registerInputs(_slices, "slices");
	registerOutput(_allSlices, "slices");
	registerOutput(_linearConstraints, "linear constraints");
}

void
StackSliceExtractor::SliceCollector::updateOutputs() {

	/*
	 * initialise
	 */

	_allSlices->clear();
	_linearConstraints->clear();

	// create a copy of the input slice collections
	std::vector<Slices> inputSlices;
	foreach (boost::shared_ptr<Slices> slices, _slices)
		inputSlices.push_back(*slices);

	// remove all duplicates from the slice collections
	inputSlices = removeDuplicates(inputSlices);

	// create outputs
	extractSlices(inputSlices);
	extractConstraints(inputSlices);

	LOG_DEBUG(stacksliceextractorlog) << _allSlices->size() << " slices found" << std::endl;
	LOG_DEBUG(stacksliceextractorlog) << _linearConstraints->size() << " consistency constraints found" << std::endl;
}

unsigned int
StackSliceExtractor::SliceCollector::countSlices(const std::vector<Slices>& slices) {

	unsigned int numSlices = 0;

	for (unsigned int level = 0; level < slices.size(); level++)
		numSlices += slices[level].size();

	return numSlices;
}

std::vector<Slices>
StackSliceExtractor::SliceCollector::removeDuplicates(const std::vector<Slices>& slices) {

	LOG_DEBUG(stacksliceextractorlog) << "removing duplicates from " << countSlices(slices) << " slices" << std::endl;

	unsigned int oldSize = 0;
	unsigned int newSize = countSlices(slices);

	std::vector<Slices> withoutDuplicates = slices;

	while (oldSize != newSize) {

		LOG_DEBUG(stacksliceextractorlog) << "current size is " << countSlices(withoutDuplicates) << std::endl;
	
		withoutDuplicates = removeDuplicatesPass(withoutDuplicates);

		LOG_DEBUG(stacksliceextractorlog) << "new size is " << countSlices(withoutDuplicates) << std::endl;

		oldSize = newSize;
		newSize = countSlices(withoutDuplicates);
	}

	LOG_DEBUG(stacksliceextractorlog) << "removed " << (countSlices(slices) - countSlices(withoutDuplicates)) << " slices" << std::endl;

	return withoutDuplicates;
}

std::vector<Slices>
StackSliceExtractor::SliceCollector::removeDuplicatesPass(const std::vector<Slices>& allSlices) {

	std::vector<Slices> slices = allSlices;

	Overlap overlap;

	// for all levels
	for (unsigned int level = 0; level < slices.size(); level++) { 

		LOG_ALL(stacksliceextractorlog) << "processing level " << level << std::endl;

		// for each slice
		foreach (boost::shared_ptr<Slice> slice, slices[level]) {

			std::vector<boost::shared_ptr<Slice> > duplicates;

			// for all sub-levels
			for (unsigned int subLevel = level + 1; subLevel < slices.size(); subLevel++) {

				LOG_ALL(stacksliceextractorlog) << "processing sub-level " << subLevel << std::endl;

				std::vector<boost::shared_ptr<Slice> > toBeRemoved;

				// for each slice
				foreach (boost::shared_ptr<Slice> subSlice, slices[subLevel]) {

					// if the overlap exceeds the threshold, store subSlice as a
					// duplicate of slice
					if (overlap(*slice, *subSlice, true, false) >= optionSimilarityThreshold.as<double>()) {

						duplicates.push_back(subSlice);
						toBeRemoved.push_back(subSlice);
					}
				}

				// remove duplicates from this level
				foreach (boost::shared_ptr<Slice> subSlice, toBeRemoved)
					slices[subLevel].remove(subSlice);
			}

			// replace slice and duplicates by their intersection
			foreach (boost::shared_ptr<Slice> duplicate, duplicates)
				slice->intersect(*duplicate);
		}
	}

	return slices;
}

void
StackSliceExtractor::SliceCollector::extractSlices(const std::vector<Slices>& slices) {

	// for all levels
	for (unsigned int level = 0; level < slices.size(); level++)
		_allSlices->addAll(slices[level]);
}

void
StackSliceExtractor::SliceCollector::extractConstraints(const std::vector<Slices>& slices) {

	Overlap overlap;

	std::vector<unsigned int> conflictIds(2);

	// for all levels
	for (unsigned int level = 0; level < slices.size(); level++) { 

		// for each slice
		foreach (boost::shared_ptr<Slice> slice, slices[level]) {

			unsigned int numOverlaps = 0;

			// for all sub-levels
			for (unsigned int subLevel = level + 1; subLevel < slices.size(); subLevel++) {

				// for each slice
				foreach (boost::shared_ptr<Slice> subSlice, slices[subLevel]) {

					// if there is overlap, add a consistency constraint
					if (overlap(*slice, *subSlice, false, false) > 0) {

						conflictIds[0] = slice->getId();
						conflictIds[1] = subSlice->getId();

						_allSlices->addConflicts(conflictIds);

						LinearConstraint linearConstraint;
						linearConstraint.setCoefficient(slice->getId(), 1.0);
						linearConstraint.setCoefficient(subSlice->getId(), 1.0);
						linearConstraint.setRelation(LessEqual);
						linearConstraint.setValue(1.0);

						_linearConstraints->add(linearConstraint);
					}
				}
			}

			// if there is no overlap with other slices, make sure that this
			// slice will be picked at most once
			if (numOverlaps == 0) {

				LinearConstraint linearConstraint;
				linearConstraint.setCoefficient(slice->getId(), 1.0);
				linearConstraint.setRelation(LessEqual);
				linearConstraint.setValue(1.0);

				_linearConstraints->add(linearConstraint);
			}
		}
	}
}
