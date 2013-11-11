// Copyright (C) 2011-2012 by the BEM++ Authors
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "modified_helmholtz_3d_hypersingular_boundary_operator.hpp"

#include "../common/boost_make_shared_fwd.hpp"

#include "abstract_boundary_operator.hpp"
#include "context.hpp"
#include "general_elementary_local_operator_imp.hpp"
#include "general_elementary_singular_integral_operator_imp.hpp"
#include "general_hypersingular_integral_operator_imp.hpp"
#include "modified_helmholtz_3d_single_layer_boundary_operator.hpp"
#include "synthetic_integral_operator.hpp"

#include "../fiber/explicit_instantiation.hpp"

#include "../fiber/modified_helmholtz_3d_hypersingular_kernel_functor.hpp"
#include "../fiber/modified_helmholtz_3d_hypersingular_kernel_interpolated_functor.hpp"
#include "../fiber/modified_helmholtz_3d_hypersingular_off_diagonal_kernel_functor.hpp"
#include "../fiber/modified_helmholtz_3d_hypersingular_off_diagonal_interpolated_kernel_functor.hpp"
#include "../fiber/modified_helmholtz_3d_hypersingular_transformation_functor.hpp"
#include "../fiber/modified_helmholtz_3d_hypersingular_integrand_functor_2.hpp"
#include "../fiber/scalar_function_value_functor.hpp"
#include "../fiber/scalar_function_value_times_normal_functor.hpp"
#include "../fiber/simple_test_scalar_kernel_trial_integrand_functor.hpp"
#include "../fiber/single_component_test_trial_integrand_functor.hpp"
#include "../fiber/surface_curl_3d_functor.hpp"

#include "../grid/max_distance.hpp"

#include <boost/type_traits/is_complex.hpp>

#include "../fmm/transmit_receive.hpp"

namespace Bempp
{

template <typename BasisFunctionType, typename KernelType, typename ResultType>
BoundaryOperator<BasisFunctionType, ResultType>
modifiedHelmholtz3dSyntheticHypersingularBoundaryOperator(
        const shared_ptr<const Context<BasisFunctionType,ResultType> >& context,
        const shared_ptr<const Space<BasisFunctionType> >& domain,
        const shared_ptr<const Space<BasisFunctionType> >& range,
        const shared_ptr<const Space<BasisFunctionType> >& dualToRange,
        KernelType waveNumber,
        std::string label,
        int internalSymmetry,
        bool useInterpolation,
        int interpPtsPerWavelength)
{
    typedef typename ScalarTraits<BasisFunctionType>::RealType CoordinateType;

    typedef Fiber::ScalarFunctionValueFunctor<CoordinateType>
            ValueFunctor;
    typedef Fiber::ScalarFunctionValueTimesNormalFunctor<CoordinateType>
            ValueTimesNormalFunctor;
    typedef Fiber::SurfaceCurl3dFunctor<CoordinateType>
            CurlFunctor;
    typedef Fiber::SingleComponentTestTrialIntegrandFunctor<
            BasisFunctionType, ResultType> IntegrandFunctor;

    typedef GeneralElementaryLocalOperator<BasisFunctionType, ResultType> LocalOp;
    typedef SyntheticIntegralOperator<BasisFunctionType, ResultType> SyntheticOp;

    if (!domain || !range || !dualToRange)
        throw std::invalid_argument(
            "modifiedHelmholtz3dSyntheticHypersingularBoundaryOperator(): "
            "domain, range and dualToRange must not be null");

    shared_ptr<const Context<BasisFunctionType, ResultType> >
        internalContext, auxContext;
    SyntheticOp::getContextsForInternalAndAuxiliaryOperators(
        context, internalContext, auxContext);
    shared_ptr<const Space<BasisFunctionType> > internalTrialSpace = 
        domain->discontinuousSpace(domain);
    shared_ptr<const Space<BasisFunctionType> > internalTestSpace = 
        dualToRange->discontinuousSpace(dualToRange);

    // Note: we don't really need to care about ranges and duals to domains of
    // the internal operator. The only range space that matters is that of the
    // leftmost operator in the product.

    const char xyz[] = "xyz";
    const size_t dimWorld = 3;

    if (label.empty())
        label = AbstractBoundaryOperator<BasisFunctionType, ResultType>::
            uniqueLabel();

    BoundaryOperator<BasisFunctionType, ResultType> slp =
            modifiedHelmholtz3dSingleLayerBoundaryOperator<
                    BasisFunctionType, KernelType, ResultType>(
                internalContext, internalTrialSpace, internalTestSpace /* or whatever */,
                internalTestSpace,
                waveNumber, "(" + label + ")_internal_SLP", internalSymmetry,
                useInterpolation, interpPtsPerWavelength);

    // symmetry of the decomposition
    int syntheseSymmetry = 0; // symmetry of the decomposition
    if (domain == dualToRange && internalTrialSpace == internalTestSpace)
        syntheseSymmetry = HERMITIAN |
                (boost::is_complex<BasisFunctionType>() ? 0 : SYMMETRIC);

    std::vector<BoundaryOperator<BasisFunctionType, ResultType> > testLocalOps;
    std::vector<BoundaryOperator<BasisFunctionType, ResultType> > trialLocalOps;
    testLocalOps.resize(dimWorld);
    for (size_t i = 0; i < dimWorld; ++i)
        testLocalOps[i] =
                BoundaryOperator<BasisFunctionType, ResultType>(
                    auxContext, boost::make_shared<LocalOp>(
                        internalTestSpace, range, dualToRange,
                        ("(" + label + ")_test_curl_") + xyz[i], NO_SYMMETRY,
                        CurlFunctor(),
                        ValueFunctor(),
                        IntegrandFunctor(i, 0)));

    if (!syntheseSymmetry) {
        trialLocalOps.resize(dimWorld);
        for (size_t i = 0; i < dimWorld; ++i)
            trialLocalOps[i] =
                    BoundaryOperator<BasisFunctionType, ResultType>(
                        auxContext, boost::make_shared<LocalOp>(
                            domain, internalTrialSpace /* or whatever */,
                            internalTrialSpace,
                            ("(" + label + ")_trial_curl_") + xyz[i], NO_SYMMETRY,
                            ValueFunctor(),
                            CurlFunctor(),
                            IntegrandFunctor(0, i)));
    }
    // It might be more prudent to distinguish between the symmetry of the total
    // operator and the symmetry of the decomposition
    BoundaryOperator<BasisFunctionType, ResultType> term0(
                context, boost::make_shared<SyntheticOp>(
                    testLocalOps, slp, trialLocalOps, label,
                    syntheseSymmetry));

    for (size_t i = 0; i < dimWorld; ++i)
        testLocalOps[i] =
                BoundaryOperator<BasisFunctionType, ResultType>(
                    auxContext, boost::make_shared<LocalOp>(
                        internalTestSpace, range, dualToRange,
                        ("(" + label + ")_test_k_value_n_") + xyz[i], NO_SYMMETRY,
                        ValueTimesNormalFunctor(),
                        ValueFunctor(),
                        IntegrandFunctor(i, 0)));
    if (!syntheseSymmetry)
        for (size_t i = 0; i < dimWorld; ++i)
            trialLocalOps[i] =
                    BoundaryOperator<BasisFunctionType, ResultType>(
                        auxContext, boost::make_shared<LocalOp>(
                            domain, internalTrialSpace /* or whatever */,
                            internalTrialSpace,
                            ("(" + label + ")_trial_k_value_n_") + xyz[i], NO_SYMMETRY,
                            ValueFunctor(),
                            ValueTimesNormalFunctor(),
                            IntegrandFunctor(0, i)));
    BoundaryOperator<BasisFunctionType, ResultType> slp_waveNumberSq =
            (waveNumber * waveNumber) * slp;
    BoundaryOperator<BasisFunctionType, ResultType> term1(
                context, boost::make_shared<SyntheticOp>(
                    testLocalOps, slp_waveNumberSq, trialLocalOps, label,
                    syntheseSymmetry));

    return term0 + term1;
}

template <typename BasisFunctionType, typename KernelType, typename ResultType>
BoundaryOperator<BasisFunctionType, ResultType>
modifiedHelmholtz3dHypersingularBoundaryOperator(
        const shared_ptr<const Context<BasisFunctionType,ResultType> >& context,
        const shared_ptr<const Space<BasisFunctionType> >& domain,
        const shared_ptr<const Space<BasisFunctionType> >& range,
        const shared_ptr<const Space<BasisFunctionType> >& dualToRange,
        KernelType waveNumber,
        const std::string& label,
        int symmetry,
        bool useInterpolation,
        int interpPtsPerWavelength)
{
    const AssemblyOptions& assemblyOptions = context->assemblyOptions();
    if (assemblyOptions.assemblyMode() == AssemblyOptions::ACA &&
         assemblyOptions.acaOptions().mode == AcaOptions::LOCAL_ASSEMBLY)
        return modifiedHelmholtz3dSyntheticHypersingularBoundaryOperator(
            context, domain, range, dualToRange, waveNumber, label, 
            symmetry, useInterpolation, interpPtsPerWavelength);

    typedef typename ScalarTraits<BasisFunctionType>::RealType CoordinateType;

    typedef Fiber::ModifiedHelmholtz3dHypersingularKernelFunctor<KernelType>
            NoninterpolatedKernelFunctor;
    typedef Fiber::ModifiedHelmholtz3dHypersingularKernelInterpolatedFunctor<KernelType>
            InterpolatedKernelFunctor;
    typedef Fiber::ModifiedHelmholtz3dHypersingularTransformationFunctor<CoordinateType>
            TransformationFunctor;
    typedef Fiber::ModifiedHelmholtz3dHypersingularIntegrandFunctor2<
            BasisFunctionType, KernelType, ResultType> IntegrandFunctor;

    typedef Fiber::ModifiedHelmholtz3dHypersingularOffDiagonalInterpolatedKernelFunctor<KernelType>
            OffDiagonalInterpolatedKernelFunctor;
    typedef Fiber::ModifiedHelmholtz3dHypersingularOffDiagonalKernelFunctor<KernelType>
            OffDiagonalNoninterpolatedKernelFunctor;
    typedef Fiber::ScalarFunctionValueFunctor<CoordinateType>
            OffDiagonalTransformationFunctor;
    typedef Fiber::SimpleTestScalarKernelTrialIntegrandFunctor<
            BasisFunctionType, KernelType, ResultType>
            OffDiagonalIntegrandFunctor;

    CoordinateType maxDistance_ =
            static_cast<CoordinateType>(1.1) *
            maxDistance(*domain->grid(), *dualToRange->grid());

    shared_ptr<FmmTransform<ResultType> > fmmTransform;
    if (assemblyOptions.assemblyMode() == AssemblyOptions::FMM) {
        const FmmOptions& fmmOptions = assemblyOptions.fmmOptions();
        fmmTransform = boost::make_shared<FmmHypersingularHighFreq<ResultType> >
            (waveNumber, fmmOptions.L, fmmOptions.levels);
    }

    typedef GeneralHypersingularIntegralOperator<
            BasisFunctionType, KernelType, ResultType> Op;
    shared_ptr<Op> newOp;
    if (useInterpolation)
        newOp.reset(new Op(
                        domain, range, dualToRange, label, symmetry,
                        InterpolatedKernelFunctor(
                            waveNumber, maxDistance_, interpPtsPerWavelength),
                        TransformationFunctor(),
                        TransformationFunctor(),
                        IntegrandFunctor(),
                        OffDiagonalInterpolatedKernelFunctor(
                            waveNumber, maxDistance_, interpPtsPerWavelength),
                        OffDiagonalTransformationFunctor(),
                        OffDiagonalTransformationFunctor(),
                        OffDiagonalIntegrandFunctor(),
                        fmmTransform));
    else
        newOp.reset(new Op(
                        domain, range, dualToRange, label, symmetry,
                        NoninterpolatedKernelFunctor(waveNumber),
                        TransformationFunctor(),
                        TransformationFunctor(),
                        IntegrandFunctor(),
                        OffDiagonalNoninterpolatedKernelFunctor(waveNumber),
                        OffDiagonalTransformationFunctor(),
                        OffDiagonalTransformationFunctor(),
                        OffDiagonalIntegrandFunctor(),
                        fmmTransform));
    return BoundaryOperator<BasisFunctionType, ResultType>(context, newOp);
}

#define INSTANTIATE_NONMEMBER_CONSTRUCTOR(BASIS, KERNEL, RESULT) \
    template BoundaryOperator<BASIS, RESULT> \
    modifiedHelmholtz3dHypersingularBoundaryOperator( \
        const shared_ptr<const Context<BASIS, RESULT> >&, \
        const shared_ptr<const Space<BASIS> >&, \
        const shared_ptr<const Space<BASIS> >&, \
        const shared_ptr<const Space<BASIS> >&, \
        KERNEL, \
        const std::string&, int, bool, int)
FIBER_ITERATE_OVER_BASIS_KERNEL_AND_RESULT_TYPES(INSTANTIATE_NONMEMBER_CONSTRUCTOR);

} // namespace Bempp
