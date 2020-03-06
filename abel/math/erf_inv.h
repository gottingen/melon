//
// Created by liyinbin on 2020/2/28.
//

#ifndef ABEL_MATH_ERF_INV_H_
#define ABEL_MATH_ERF_INV_H_

#include <abel/math/option.h>
#include <abel/math/exp.h>
#include <abel/math/max.h>
#include <abel/math/min.h>
#include <abel/math/abs.h>
#include <abel/math/erf.h>

namespace abel {

    namespace math_internal {

        template<typename T>
        ABEL_CONSTEXPR T erf_inv_decision(const T value, const T p, const T direc, const int iter_count) ABEL_NOEXCEPT;


        template<typename T>
        ABEL_CONSTEXPR T erf_inv_initial_val_coef_2(const T a, const T p_term, const int order) ABEL_NOEXCEPT {
            return( order == 1 ? T(-0.000200214257L) :
                    order == 2 ? T( 0.000100950558L) + a*p_term :
                    order == 3 ? T( 0.00134934322L)  + a*p_term :
                    order == 4 ? T(-0.003673428440L) + a*p_term :
                    order == 5 ? T( 0.005739507730L) + a*p_term :
                    order == 6 ? T(-0.00762246130L)  + a*p_term :
                    order == 7 ? T( 0.009438870470L) + a*p_term :
                    order == 8 ? T( 1.001674060000L) + a*p_term :
                    order == 9 ? T( 2.83297682000L)  + a*p_term :
                    p_term );
        }

        template<typename T>
        ABEL_CONSTEXPR T erf_inv_initial_val_case_2(const T a, const T p_term, const int order) ABEL_NOEXCEPT {
            return( order == 9 ? erf_inv_initial_val_coef_2(a,p_term,order) : erf_inv_initial_val_case_2(a,erf_inv_initial_val_coef_2(a,p_term,order),order+1) );
        }

        template<typename T>
        ABEL_CONSTEXPR T erf_inv_initial_val_coef_1(const T a, const T p_term, const int order) ABEL_NOEXCEPT {
            return( order == 1 ? T( 2.81022636e-08L) :
                    order == 2 ? T( 3.43273939e-07L) + a*p_term :
                    order == 3 ? T(-3.5233877e-06L)  + a*p_term :
                    order == 4 ? T(-4.39150654e-06L) + a*p_term :
                    order == 5 ? T( 0.00021858087L)  + a*p_term :
                    order == 6 ? T(-0.00125372503L)  + a*p_term :
                    order == 7 ? T(-0.004177681640L) + a*p_term :
                    order == 8 ? T( 0.24664072700L)  + a*p_term :
                    order == 9 ? T( 1.50140941000L)  + a*p_term :
                    p_term );
        }

        template<typename T>
        ABEL_CONSTEXPR T erf_inv_initial_val_case_1(const T a, const T p_term, const int order) ABEL_NOEXCEPT {
            return( order == 9 ? erf_inv_initial_val_coef_1(a,p_term,order) : erf_inv_initial_val_case_1(a,erf_inv_initial_val_coef_1(a,p_term,order),order+1) );
        }

        template<typename T>
        ABEL_CONSTEXPR T erf_inv_initial_val_int(const T a) ABEL_NOEXCEPT {
            return( a < T(5) ? erf_inv_initial_val_case_1(a-T(2.5),T(0),1) : erf_inv_initial_val_case_2(sqrt(a)-T(3),T(0),1) );
        }

        template<typename T>
        ABEL_CONSTEXPR T erf_inv_initial_val(const T x) ABEL_NOEXCEPT {
            return x*erf_inv_initial_val_int( -log( (T(1) - x)*(T(1) + x) ) );
        }



        template<typename T>
        ABEL_CONSTEXPR T erf_inv_err_val(const T value, const T p) ABEL_NOEXCEPT {
            return( erf(value) - p );
        }

        template<typename T>
        ABEL_CONSTEXPR T erf_inv_deriv_1(const T value) ABEL_NOEXCEPT {
            return( exp( -value*value ) );
        }

        template<typename T>
        ABEL_CONSTEXPR T erf_inv_deriv_2(const T value, const T deriv_1) ABEL_NOEXCEPT {
            return( deriv_1*( -T(2)*value ) );
        }

        template<typename T>
        ABEL_CONSTEXPR T erf_inv_ratio_val_1(const T value, const T p, const T deriv_1) ABEL_NOEXCEPT {
            return( erf_inv_err_val(value,p) / deriv_1 );
        }

        template<typename T>
        ABEL_CONSTEXPR T erf_inv_ratio_val_2(const T value, const T deriv_1) ABEL_NOEXCEPT {
            return( erf_inv_deriv_2(value,deriv_1) / deriv_1 );
        }

        template<typename T>
        ABEL_CONSTEXPR T erf_inv_halley(const T ratio_val_1, const T ratio_val_2) ABEL_NOEXCEPT {
            return( ratio_val_1 / max( T(0.8), min( T(1.2), T(1) - T(0.5)*ratio_val_1*ratio_val_2 ) ) );
        }

        template<typename T>
        ABEL_CONSTEXPR T erf_inv_recur(const T value, const T p, const T deriv_1, const int iter_count) ABEL_NOEXCEPT {
            return erf_inv_decision( value, p,
                                     erf_inv_halley(erf_inv_ratio_val_1(value,p,deriv_1),
                                                    erf_inv_ratio_val_2(value,deriv_1)),
                                     iter_count );
        }

        template<typename T>
        ABEL_CONSTEXPR T erf_inv_decision(const T value, const T p, const T direc, const int iter_count) ABEL_NOEXCEPT {
            return( iter_count < ABEL_ERF_INV_MAX_ITER ?
                    erf_inv_recur(value-direc,p, erf_inv_deriv_1(value), iter_count+1) :
                    value - direc );
        }

        template<typename T>
        ABEL_CONSTEXPR T erf_inv_recur_begin(const T initial_val, const T p) ABEL_NOEXCEPT {
            return erf_inv_recur(initial_val,p,erf_inv_deriv_1(initial_val),1);
        }

        template<typename T>
        ABEL_CONSTEXPR T erf_inv_begin(const T p) ABEL_NOEXCEPT {
            return( is_nan(p) ? std::numeric_limits<T>::quiet_NaN() : abs(p) > T(1) ?
                std::numeric_limits<T>::quiet_NaN() :
                    std::numeric_limits<T>::epsilon() > abs(T(1) - p) ? std::numeric_limits<T>::infinity() :
                    std::numeric_limits<T>::epsilon() > abs(T(1) + p) ? - std::numeric_limits<T>::infinity() :
                    erf_inv_recur_begin(erf_inv_initial_val(p),p) );
        }

    } //namespace math_internal


    template<typename T>
    ABEL_DEPRECATED_MESSAGE("use std version instead")
    ABEL_CONSTEXPR return_t<T> erf_inv(const T p) ABEL_NOEXCEPT {
        return math_internal::erf_inv_begin( static_cast<return_t<T>>(p) );
    }


}
#endif //ABEL_MATH_ERF_INV_H_
