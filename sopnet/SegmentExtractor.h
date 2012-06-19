#ifndef CELLTRACKER_TRACKLET_EXTRACTOR_H__
#define CELLTRACKER_TRACKLET_EXTRACTOR_H__

#include <boost/function.hpp>

#include <pipeline/all.h>
#include <inference/LinearConstraints.h>
#include "Slices.h"
#include "Segments.h"

class SegmentExtractor : public pipeline::SimpleProcessNode {

public:

	SegmentExtractor();

private:

	void updateOutputs();

	void extractSegments();

	void extractSegment(boost::shared_ptr<Slice> prevSlice, Direction direction);

	void extractSegment(boost::shared_ptr<Slice> prevSlice, boost::shared_ptr<Slice> nextSlice);

	void assembleLinearConstraints();

	void assembleLinearConstraint(const LinearConstraint& sliceConstraint);

	// the slices
	pipeline::Input<Slices> _prevSlices;
	pipeline::Input<Slices> _nextSlices;

	// the linear constraints on the slices
	pipeline::Input<LinearConstraints> _prevLinearConstraints;
	pipeline::Input<LinearConstraints> _nextLinearConstraints;

	// the distance in pixels until which to consider slices to belong to the
	// same segment
	pipeline::Input<double> _distanceThreshold;

	// the extracted segments and the linear constraints on them
	pipeline::Output<Segments>          _segments;
	pipeline::Output<LinearConstraints> _linearConstraints;

	// a map from slice ids to segments (ids) they are used in
	std::map<unsigned int, std::vector<unsigned int> > _sliceSegments;
};

#endif // CELLTRACKER_TRACKLET_EXTRACTOR_H__

