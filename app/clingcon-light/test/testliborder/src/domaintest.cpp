// {{{ GPL License

// This file is part of libcsp - a library for handling linear constraints.
// Copyright (C) 2016  Max Ostrowski

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

// }}}

#include "catch.hpp"
#include "order/domain.h"
#include "order/config.h"
#include <iostream>

using namespace order;

    TEST_CASE("Domain addition", "[addition]")
    {
        Domain d(0,100);
        d.unify(200,300);
        d.unify(-100,-50);
        REQUIRE(d.size()==253);
        Domain::const_iterator begin = d.begin();
        Domain::const_iterator end = d.end();
        int count = 0;
        for (auto i = begin; i != end; ++i)
        {
            ++count;
            if (count==57)
            {
                REQUIRE(*i==5);
            }
        }
        REQUIRE(count==253);
        REQUIRE(end-begin==253);
        REQUIRE(*(begin+50)==-50);
        REQUIRE(*(begin+51)==0);
        REQUIRE(*(begin+76)==25);
        REQUIRE(*(begin+252)==300);

        REQUIRE(*(std::lower_bound(begin, end, 20))==20);
        REQUIRE(*(std::lower_bound(begin, end, -20))==0);
        REQUIRE(*(std::lower_bound(begin, end, -30))==0);
        REQUIRE(*(std::lower_bound(begin, end, -50))==-50);
        REQUIRE(*(std::lower_bound(begin, end, 75))==75);
        REQUIRE(*(std::lower_bound(begin, end, 100))==100);
        REQUIRE(*(std::lower_bound(begin, end, 200))==200);
        REQUIRE(*(std::lower_bound(begin, end, 150))==200);
        REQUIRE(*(std::lower_bound(begin, end, 101))==200);
        REQUIRE((std::lower_bound(begin, end, 1500))==end);


        Domain e(0,100);
        e.unify(-100,50);
        Domain::const_iterator begin2 = e.begin();
        Domain::const_iterator end2 = e.end();

        REQUIRE(end2-begin2==201);
        REQUIRE(e.size()==201);


        Domain f(0,0);
        f.unify(d);
        REQUIRE(f==d);
        f.unify(e);

        Domain g(-100,100);
        g.unify(200,300);
        REQUIRE(g==f);

        g.unify(101,199);
        REQUIRE(g.size()==401);


        unsigned int domSize=10000;
        Domain h(-10,5);
        h.inplace_times(5, domSize);
        REQUIRE(*h.begin()==-50);
        REQUIRE(*(h.end()-1)==25);
        h.unify(50,75);
        h.inplace_times(-3, domSize);
        REQUIRE(*h.begin()==-225);
        REQUIRE(*(h.end()-1)==150);
        //std::cout << h << std::endl;

        e+=h;

        REQUIRE(*e.begin()==-325);
        REQUIRE(*(e.end()-1)==250);

        Domain i(1,1);
        i.unify(5,6);

        REQUIRE(h+i==i+h);
        i+=h;
        REQUIRE(*i.begin()==-224);
        REQUIRE(*(i.end()-1)==156);

        Domain j(1,1);
        j.unify(6,8);
        j.unify(2,5);

        REQUIRE(j.getRanges().size()==1);
        REQUIRE(j.getRanges().back()==Range(1,8));
    }

    TEST_CASE("Domain sets", "[sets]")
    {
        unsigned int domSize=10000;
        Domain d(0,100);
        d.inplace_times(5, domSize);
        d.inplace_times(-43, domSize);
        REQUIRE(d.size()==101);

        Domain e(1,100);
        e += Domain(3,3);

        REQUIRE(e.lower()==4);
        REQUIRE(e.upper()==103);
        REQUIRE(e.size()==100);

        e += Domain(1000,1999);
        REQUIRE(e.size()==1099);

        Domain f(0,99);
        f.intersect(50,70);
        REQUIRE(f.lower()==50);
        REQUIRE(f.upper()==70);
        f.intersect(65,60);
        REQUIRE(f.empty());

        Domain g(0,99);
        g.remove(10,19);
        g.remove(50,59);
        g.remove(20,29);
        REQUIRE(g.size()==100-30);
        g.remove(70,79);
        g.remove(65,69);
        REQUIRE(g.size()==100-30-15);
        g.remove(7,62);
        REQUIRE(g.size()==29);
        g.intersect(23,72);
        REQUIRE(g.size()==2);
        g.unify(0,99);
        REQUIRE(g.size()==100);


        Domain h(0,13);
        h.unify(22,34);
        h.unify(58,96);
        h.unify(1000,1000);
        h.unify(34,-27);

        Domain i(-17,4);
        i.unify(97,97);
        i.unify(27,29);
        i.unify(30,60);

        h.unify(i);

        Domain erg(-17,13);
        erg.unify(22,97);
        erg.unify(1000,1000);
        REQUIRE(erg==h);


        Domain k(0,1000);
        k.remove(101,899);
        Domain j(5,10);
        j.unify(20,30);
        j.unify(40,50);
        k.intersect(j);
        REQUIRE(k.size()==28);

    }

    TEST_CASE("Domain lower_bound", "[lower_bound]")
    {
        {
        Domain d(1,100);

        Restrictor r(View(0,-1,0),d);

        REQUIRE(r.begin() < r.end());

        }

        Domain d(0,100);
        d.unify(200,300);
        d.unify(-100,-50);

        {
        Restrictor r(View(0,1,0),d);

        REQUIRE(*(order::wrap_lower_bound(r.begin(), r.end(), 50))==50);
        REQUIRE(*(order::wrap_lower_bound(r.begin(), r.end(), 0))==0);
        REQUIRE(*(order::wrap_lower_bound(r.begin(), r.end(), -1))==0);
        REQUIRE(*(order::wrap_lower_bound(r.begin(), r.end(), -101))==-100);
        REQUIRE(*(order::wrap_lower_bound(r.begin(), r.end(), -500))==-100);
        REQUIRE(*(order::wrap_lower_bound(r.begin(), r.end(), -50))==-50);
        REQUIRE(*(order::wrap_lower_bound(r.begin(), r.end(), 300))==300);
        REQUIRE(order::wrap_lower_bound(r.begin(), r.end(), 301)==r.end());
        REQUIRE(order::wrap_lower_bound(r.begin(), r.end(), 12030)==r.end());
        }

        {
        Restrictor r(View(0,1,1000),d);

        REQUIRE(*(order::wrap_lower_bound(r.begin(), r.end(), 1000+ 50))  ==1000+ 50);
        REQUIRE(*(order::wrap_lower_bound(r.begin(), r.end(), 1000+ 0))   ==1000+ 0);
        REQUIRE(*(order::wrap_lower_bound(r.begin(), r.end(), 1000+ -1))  ==1000+ 0);
        REQUIRE(*(order::wrap_lower_bound(r.begin(), r.end(), 1000+ -101))==1000+ -100);
        REQUIRE(*(order::wrap_lower_bound(r.begin(), r.end(), 1000+ -500))==1000+ -100);
        REQUIRE(*(order::wrap_lower_bound(r.begin(), r.end(), 1000+ -50)) ==1000+ -50);
        REQUIRE(*(order::wrap_lower_bound(r.begin(), r.end(), 1000+ 300)) ==1000+ 300);
        REQUIRE(order::wrap_lower_bound(r.begin(), r.end(), 1000+ 301)==r.end());
        REQUIRE(order::wrap_lower_bound(r.begin(), r.end(), 1000+ 12030)==r.end());
        }

        {
        Restrictor r(View(0,-1,0),d);


        REQUIRE(*(order::wrap_lower_bound(r.begin(), r.end(), -300))==-300);
        REQUIRE(*(order::wrap_lower_bound(r.begin(), r.end(), -200))==-200);
        REQUIRE(*(order::wrap_lower_bound(r.begin(), r.end(), -199))==-100);
        REQUIRE(*(order::wrap_lower_bound(r.begin(), r.end(), -201))==-201);
        REQUIRE(*(order::wrap_lower_bound(r.begin(), r.end(), -100))==-100);
        REQUIRE(*(order::wrap_lower_bound(r.begin(), r.end(), -101))==-100);
        REQUIRE(*(order::wrap_lower_bound(r.begin(), r.end(), -99))==-99);
        REQUIRE(*(order::wrap_lower_bound(r.begin(), r.end(), -1))==-1);
        REQUIRE(*(order::wrap_lower_bound(r.begin(), r.end(), 0))==0);
        REQUIRE(*(order::wrap_lower_bound(r.begin(), r.end(), 1))==50);
        REQUIRE(*(order::wrap_lower_bound(r.begin(), r.end(), 25))==50);
        REQUIRE(*(order::wrap_lower_bound(r.begin(), r.end(), 50))==50);
        REQUIRE(*(order::wrap_lower_bound(r.begin(), r.end(), 0))==0);
        REQUIRE(*(order::wrap_lower_bound(r.begin(), r.end(), -1))==-1);
        REQUIRE(*(order::wrap_lower_bound(r.begin(), r.end(), 1))==50);
        REQUIRE(*(order::wrap_lower_bound(r.begin(), r.end(), -101))==-100);
        REQUIRE(*(order::wrap_lower_bound(r.begin(), r.end(), -500))==-300);
        REQUIRE(*(order::wrap_lower_bound(r.begin(), r.end(), -50))==-50);
        REQUIRE(*(order::wrap_lower_bound(r.begin(), r.end(), 100))==100);
        REQUIRE(order::wrap_lower_bound(r.begin(), r.end(), 101)==r.end());
        REQUIRE(order::wrap_lower_bound(r.begin(), r.end(), 12030)==r.end());
        }

    }

