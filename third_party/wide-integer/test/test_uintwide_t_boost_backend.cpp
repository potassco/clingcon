///////////////////////////////////////////////////////////////////////////////
//  Copyright Christopher Kormanyos 2019 - 2021.
//  Distributed under the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt
//  or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

#if (defined(__clang__) && (__clang_major__ > 9)) && !defined(__APPLE__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-copy"
#endif

#include <boost/multiprecision/cpp_int.hpp>
#include <boost/multiprecision/uintwide_t_backend.hpp>

#include <test/test_uintwide_t.h>

typedef boost::multiprecision::number<boost::multiprecision::uintwide_t_backend<1024U>, boost::multiprecision::et_off>
    local_uint_type;

using boost_uint_backend_type =
    boost::multiprecision::cpp_int_backend<1024, 1024, boost::multiprecision::unsigned_magnitude>;

using boost_uint_type = boost::multiprecision::number<boost_uint_backend_type, boost::multiprecision::et_off>;

bool math::wide_integer::test_uintwide_t_boost_backend() {
    bool result_is_ok = true;

    // Test a non-trivial calculation. A naive algorithm for calculating
    // a factorial (in this case 100!) has been selected.
    {
        local_uint_type u = 1U;

        for (std::size_t i = 2U; i <= 100U; ++i) {
            u *= i;
        }

        const local_uint_type local_control(
            "9332621544394415268169923885626670049071596826438162146859296389521759999322991560894146397615651828625369"
            "7920827223758251185210916864000000000000000000000000");
        const boost_uint_type boost_control(
            "9332621544394415268169923885626670049071596826438162146859296389521759999322991560894146397615651828625369"
            "7920827223758251185210916864000000000000000000000000");

        const bool local_control_is_ok = (u == local_control);
        const bool boost_control_is_ok =
            (boost::lexical_cast<std::string>(u) == boost::lexical_cast<std::string>(boost_control));

        result_is_ok &= (local_control_is_ok && boost_control_is_ok);

        // Test divide-by-limb.
        u /= 10U;

        result_is_ok &=
            (u == local_uint_type("933262154439441526816992388562667004907159682643816214685929638952175999932299156089"
                                  "4146397615651828625369792082722375825118521091686400000000000000000000000"));

        // Test full multiplication.
        u *= u;

        result_is_ok &=
            (u == local_uint_type("870978248908948007941659016194448586556972064394084013421593253624337999634658332587"
                                  "796709633275492064469038076221960747636428941143592019057396067750788139460748990533"
                                  "172975801343299298718476460737588943431348338296680151515628085416269176619573749317"
                                  "34536035195944960000000000000000000000000000000000000000000000"));
    }

    // Test a very simple constexpr example.
    {
        WIDE_INTEGER_CONSTEXPR local_uint_type cu("123");

        WIDE_INTEGER_CONSTEXPR bool result_cu_is_ok = (cu == 123U);

        result_is_ok &= result_cu_is_ok;

#if defined(WIDE_INTEGER_CONSTEXPR_IS_COMPILE_TIME_CONST) && (WIDE_INTEGER_CONSTEXPR_IS_COMPILE_TIME_CONST != 0)
        static_assert(result_cu_is_ok == true, "Error: test_uintwide_t_boost_backend not OK!");
#endif
    }

    return result_is_ok;
}

#if (defined(__clang__) && (__clang_major__ > 9)) && !defined(__APPLE__)
#pragma GCC diagnostic pop
#endif

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop
#endif
