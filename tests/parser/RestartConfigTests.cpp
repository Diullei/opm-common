/*
  Copyright 2016 Statoil ASA.

  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.
 */



#define BOOST_TEST_MODULE RestartConfigTests

#include <boost/test/unit_test.hpp>


#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Parser/ParseContext.hpp>
#include <opm/parser/eclipse/Parser/ErrorGuard.hpp>
#include <opm/parser/eclipse/EclipseState/IOConfig/RestartConfig.hpp>
#include <opm/parser/eclipse/Utility/Functional.hpp>
#include <opm/common/utility/OpmInputError.hpp>

inline std::string fst( const std::pair< std::string, int >& p ) {
    return p.first;
}

constexpr std::pair<std::time_t, std::size_t> restart_info = std::make_pair(std::time_t{0}, std::size_t{0});

using namespace Opm;

BOOST_AUTO_TEST_CASE(RPTRST_AND_RPTSOL_SOLUTION)
{
    const auto input = std::string { R"(RUNSPEC
DIMENS
  10 10 10 /
START
  6 JLY 2020 /
GRID

DXV
  10*1 /

DYV
  10*1 /

DZV
  10*1 /

DEPTH
  121*1 /

PORO
  1000*0.25 /

SOLUTION
RPTSOL
  'RESTART=2' 'FIP=3' 'FIPRESV' 'THPRES' /
SCHEDULE
RPTRST
  'BASIC=5' 'FREQ=6' 'CONV=10' /
--SCHEDULE
DATES
  7 'JLY' 2020 /          ( 1)
 10 'JLY' 2020 /          ( 2)
 20 'JLY' 2020 /          ( 3)
 30 'JLY' 2020 /          ( 4)
  5 'AUG' 2020 /          ( 5)
 20 'AUG' 2020 /          ( 6)
  5 'SEP' 2020 /          ( 7)
  1 'OCT' 2020 /          ( 8)
  1 'NOV' 2020 /          ( 9)
  1 'DEC' 2020 /          (10)
  5 'JAN' 2021 / -- WRITE (11)
  1 'FEB' 2021 /          (12)
 17 'MAY' 2021 /          (13)
  6 'JLY' 2021 / -- WRITE (14)
  1 'DEC' 2021 /          (15)
 31 'DEC' 2021 /          (16)
 21 'JAN' 2022 / -- WRITE (17)
 31 'JAN' 2022 /          (18)
/
END
)" };

    const auto deck   = Parser{}.parseString(input);
    const auto tm     = TimeMap { deck };
    const auto rstcfg = RestartConfig { deck, restart_info, {} };

    BOOST_REQUIRE_EQUAL(tm.size(), std::size_t{19});

    for (const std::size_t stepID : { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 12, 13, 15, 16, 18 }) {
        BOOST_CHECK_MESSAGE(! rstcfg.getWriteRestartFile(stepID),
                            "Must not write restart information for excluded step " << stepID);
    }

    for (const std::size_t stepID : { 0, 11, 14, 17 }) {
        BOOST_CHECK_MESSAGE(rstcfg.getWriteRestartFile(stepID),
                            "Must write restart information for included step " << stepID);
    }
}


BOOST_AUTO_TEST_CASE(RPTRST_AND_RPTSOL_SOLUTION2)
{
    const auto input = std::string { R"(RUNSPEC
DIMENS
  10 10 10 /
START
  6 JLY 2019 /
GRID

DXV
  10*1 /

DYV
  10*1 /

DZV
  10*1 /

DEPTH
  121*1 /

PORO
  1000*0.25 /
SOLUTION
-- basic = 5, every month
RPTRST
 'BASIC=5'  'FREQ=6'   'CONV=10' /

RPTSOL
 'RESTART=2'  'FIP=3'  'FIPRESV'  'THPRES' /

SCHEDULE

DATES
  1 AUG 2019 /        ( 1)
  2 AUG 2019 /        ( 2)
  3 AUG 2019 /        ( 3)
  4 AUG 2019 /        ( 4)
 12 AUG 2019 /        ( 5)
 13 AUG 2019 /        ( 6)
 14 AUG 2019 /        ( 7)
 22 AUG 2019 /        ( 8)
 23 AUG 2019 /        ( 9)
 24 AUG 2019 /        (10)
  1 SEP 2019 /        (11)
 11 SEP 2019 /        (12)
 21 SEP 2019 /        (13)
 22 SEP 2019 /        (14)
 23 SEP 2019 /        (15)
  1 OCT 2019 /        (16)
  2 OCT 2019 /        (17)
  3 OCT 2019 /        (18)
 11 OCT 2019 /        (19)
 12 OCT 2019 /        (20)
 13 OCT 2019 /        (21)
 21 OCT 2019 /        (22)
 31 OCT 2019 /        (23)
  1 NOV 2019 /        (24)
  2 NOV 2019 /        (25)
 10 NOV 2019 /        (26)
 20 NOV 2019 /        (27)
 21 NOV 2019 /        (28)
 22 NOV 2019 /        (29)
 30 NOV 2019 /        (30)
  1 DEC 2019 /        (31)
 11 DEC 2019 /        (32)
 12 DEC 2019 /        (33)
 13 DEC 2019 /        (34)
 22 DEC 2019 /        (35)
 23 DEC 2019 /        (36)
 24 DEC 2019 /        (37)
  1 JAN 2020 / Write  (38)
  2 JAN 2020 /        (39)
 12 JAN 2020 /        (40)
 13 JAN 2020 /        (41)
 14 JAN 2020 /        (42)
 23 JAN 2020 /        (43)
 24 JAN 2020 /        (44)
 25 JAN 2020 /        (45)
  1 FEB 2020 /        (46)
  1 MAR 2020 /        (47)
  1 APR 2020 /        (48)
  1 MAY 2020 /        (49)
  1 JUN 2020 /        (50)
  1 JUL 2020 / Write  (51)
  1 AUG 2020 /        (52)
  1 SEP 2020 /        (53)
  1 OCT 2020 /        (54)
  1 NOV 2020 /        (55)
/

END
)" };

    const auto deck   = Parser{}.parseString(input);
    const auto tm     = TimeMap { deck };
    const auto rstcfg = RestartConfig { deck, restart_info, {} };

    for (std::size_t step = 0; step < tm.size(); step++) {
        if (step == 0 || step == 38 || step == 51)
            BOOST_CHECK_MESSAGE( rstcfg.getWriteRestartFile(step), "Restart file expected for step: " << step );
        else
            BOOST_CHECK_MESSAGE( !rstcfg.getWriteRestartFile(step), "Should *not* have restart file for step: " << step );
    }
}


BOOST_AUTO_TEST_CASE(RPTSCHED_INTEGER) {

    const std::string deckData1 = R"(
RUNSPEC
START             -- 0
19 JUN 2007 /
DIMENS
 10 10 10 /
GRID

DXV
  10*1 /

DYV
  10*1 /

DZV
  10*1 /

DEPTH
  121*1 /

PORO
  1000*0.25 /
SOLUTION
RPTRST  -- PRES,DEN,PCOW,PCOG,RK,VELOCITY,COMPRESS
  6*0 1 0 1 9*0 1 7*0 1 0 3*1 /
SCHEDULE
DATES             -- 1
 10  OKT 2008 /
/
RPTSCHED
RESTART=1
/
DATES             -- 2
 20  JAN 2010 /
/
RPTRST  -- RK,VELOCITY,COMPRESS
  18*0 0 8*0 /
DATES             -- 3
 20  FEB 2010 /
/
RPTSCHED
RESTART=0
/
)";

    Parser parser;

    auto deck1 = parser.parseString( deckData1);
    RestartConfig rstConfig1( deck1, restart_info, {});

    BOOST_CHECK(  rstConfig1.getWriteRestartFile( 0 ) );
    BOOST_CHECK( !rstConfig1.getWriteRestartFile( 1 ) );
    BOOST_CHECK(  rstConfig1.getWriteRestartFile( 2 ) );
    BOOST_CHECK(  !rstConfig1.getWriteRestartFile( 3 ) );

    std::vector< std::string > kw_list1;
    for( const auto& pair : rstConfig1.getRestartKeywords( 1 ) )
        if( pair.second != 0 ) kw_list1.push_back( pair.first );

    const auto expected1 = {"BG","BO","BW","COMPRESS","DEN","KRG","KRO","KRW","PCOG","PCOW","PRES","RK","VELOCITY","VGAS","VOIL","VWAT"};
    BOOST_CHECK_EQUAL_COLLECTIONS( expected1.begin(), expected1.end(),
                                   kw_list1.begin(), kw_list1.end() );

    // ACIP is a valid mneonic - but not in this deck.
    BOOST_CHECK_EQUAL( rstConfig1.getKeyword( "ACIP" , 1) , 0 );
    BOOST_CHECK_EQUAL( rstConfig1.getKeyword( "COMPRESS" , 1) , 1 );
    BOOST_CHECK_EQUAL( rstConfig1.getKeyword( "PCOG", 1) , 1 );
    BOOST_CHECK_THROW( rstConfig1.getKeyword( "UNKNOWN_KW", 1) , std::invalid_argument);

    std::vector< std::string > kw_list2;
    for( const auto& pair : rstConfig1.getRestartKeywords( 3 ) )
        if( pair.second != 0 ) kw_list2.push_back( pair.first );

    const auto expected2 = { "COMPRESS", "RESTART", "RK", "VELOCITY" };
    BOOST_CHECK_EQUAL_COLLECTIONS( expected2.begin(), expected2.end(),
                                   kw_list2.begin(), kw_list2.end() );

    BOOST_CHECK_EQUAL( rstConfig1.getKeyword( "ALLPROPS" , 0 ) , 0);
    BOOST_CHECK_EQUAL( rstConfig1.getKeyword( "ALLPROPS" , 3 ) , 0);
}


const std::string& deckStr =  R"(
RUNSPEC

DIMENS
 10 10 10 /
GRID

DXV
  10*1 /

DYV
  10*1 /

DZV
  10*1 /

DEPTH
  121*1 /

PORO
  1000*0.25 /

GRIDFILE
 0 1 /

START
 21 MAY 1981 /

SCHEDULE
DATES
 22 MAY 1981 /              -- timestep 1
 23 MAY 1981 /              -- timestep 2
 24 MAY 1981 /              -- timestep 3
 25 MAY 1981 /              -- timestep 4
 26 MAY 1981 /              -- timestep 5
 1 JAN 1982 /               -- timestep 6
 1 JAN 1982 13:55:44 /      -- timestep 7
 3 JAN 1982 14:56:45.123 /  -- timestep 8
 4 JAN 1982 14:56:45.123 /  -- timestep 9
 5 JAN 1982 14:56:45.123 /  -- timestep 10
 6 JAN 1982 14:56:45.123 /  -- timestep 11
 7 JAN 1982 14:56:45.123 /  -- timestep 12
 8 JAN 1982 14:56:45.123 /  -- timestep 13
 9 JAN 1982 14:56:45.123 /  -- timestep 14
 10 JAN 1982 14:56:45.123 / -- timestep 15
 11 JAN 1982 14:56:45.123 / -- timestep 16
 1 JAN 1983 /               -- timestep 17
 2 JAN 1983 /               -- timestep 18
 3 JAN 1983 /               -- timestep 19
 1 JAN 1984 /               -- timestep 20
 2 JAN 1984 /               -- timestep 21
 1 JAN 1985 /               -- timestep 22
 3 JAN 1986 14:56:45.123 /  -- timestep 23
 4 JAN 1986 14:56:45.123 /  -- timestep 24
 5 JAN 1986 14:56:45.123 /  -- timestep 25
 1 JAN 1987 /               -- timestep 26
 1 JAN 1988 /               -- timestep 27
 2 JAN 1988 /               -- timestep 28
 3 JAN 1988 /               -- timestep 29
 1 JAN 1989 /               -- timestep 30
 2 JAN 1989 /               -- timestep 31
 2 JAN 1990 /               -- timestep 32
 2 JAN 1991 /               -- timestep 33
 3 JAN 1991 /               -- timestep 34
 4 JAN 1991 /               -- timestep 35
 1 JAN 1992 /               -- timestep 36
 1 FEB 1992 /               -- timestep 37
 1 MAR 1992 /               -- timestep 38
 2 MAR 1992 /               -- timestep 39
 3 MAR 1992 /               -- timestep 40
 4 MAR 1992 /               -- timestep 41
 1 APR 1992 /               -- timestep 42
 2 APR 1992 /               -- timestep 43
 1 MAY 1992 /               -- timestep 44
 2 MAY 1992 /               -- timestep 45
 3 MAY 1992 /               -- timestep 46
 3 JUN 1992 /               -- timestep 47
 3 JUL 1992 /               -- timestep 48
 3 AUG 1992 /               -- timestep 49
 4 AUG 1992 /               -- timestep 50
 5 AUG 1992 /               -- timestep 51
 6 AUG 1992 /               -- timestep 52
 7 AUG 1992 /               -- timestep 53
 8 AUG 1992 /               -- timestep 54
 9 AUG 1992 /               -- timestep 55
 10 AUG 1992 /              -- timestep 56
 11 AUG 1992 /              -- timestep 57
 12 AUG 1992 /              -- timestep 58
 13 AUG 1992 /              -- timestep 59
 14 AUG 1992 /              -- timestep 60
 15 AUG 1992 /              -- timestep 61
/
)";

const std::string deckStr_RFT = R"(
RUNSPEC
OIL
GAS
WATER
DIMENS
 10 10 10 /
GRID
DXV
10*0.25 /
DYV
10*0.25 /
DZV
10*0.25 /
TOPS
100*0.25 /

"START             -- 0
1 NOV 1979 /
SCHEDULE
DATES             -- 1
 1 DES 1979/
/
WELSPECS
    'OP_1'       'OP'   9   9 1*     'OIL' 1*      1*  1*   1*  1*   1*  1*  /
    'OP_2'       'OP'   4   4 1*     'OIL' 1*      1*  1*   1*  1*   1*  1*  /
/
COMPDAT
 'OP_1'  9  9   1   1 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 /
 'OP_1'  9  9   2   2 'OPEN' 1*   46.825   0.311  4332.346 1*  1*  'X'  22.123 /
 'OP_1'  9  9   3  9 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 /
 'OP_2'  4  4   4  9 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 /
/
DATES             -- 2
 10  OKT 2008 /
/
WRFT
/
WELOPEN
 'OP_1' OPEN /
 'OP_2' OPEN /
/
DATES             -- 3
 10  NOV 2008 /
/
)";



BOOST_AUTO_TEST_CASE(RPTRST_mixed_mnemonics_int_list) {
    const char* data = R"(
RUNSPEC
DIMENS
 10 10 10 /
GRID

DXV
  10*1 /

DYV
  10*1 /

DZV
  10*1 /

DEPTH
  121*1 /

PORO
  1000*0.25 /

START             -- 0
19 JUN 2007 /
SCHEDULE
DATES             -- 1
 10  OKT 2008 /
/
RPTRST
BASIC=3 0 1 2
/
DATES             -- 2
 20  JAN 2010 /
/
DATES             -- 3
 20  FEB 2010 /
/
RPTSCHED
BASIC=1
/
)";

    ParseContext parseContext;
    ErrorGuard errors;
    auto deck = Parser().parseString( data, parseContext, errors );
    parseContext.update(ParseContext::RPT_MIXED_STYLE, InputError::THROW_EXCEPTION);
    BOOST_CHECK_THROW( RestartConfig( deck, restart_info, {}, parseContext, errors ), OpmInputError);
}

BOOST_AUTO_TEST_CASE(RPTRST) {

    const std::string deckData1 = R"(
RUNSPEC
DIMENS
 10 10 10 /
GRID
START             -- 0
19 JUN 2007 /
SOLUTION
RPTRST
 ACIP KRG KRO KRW NORST SFREQ=10 ALLPROPS/
SCHEDULE
DATES             -- 1
 10  OKT 2008 /
/
RPTRST
BASIC=1
/
DATES             -- 2
 20  JAN 2010 /
/
)";

    const std::string deckData2 = R"(
RUNSPEC
DIMENS
 10 10 10 /
GRID
START             -- 0
19 JUN 2007 /
SCHEDULE
DATES             -- 1
 10  OKT 2008 /
/
RPTRST
BASIC=3 FREQ=2 FLOWS RUBBISH=5
/
DATES             -- 2
 20  JAN 2010 /
/
DATES             -- 3
 20  JAN 2011 /
/
)";

    const char* deckData3 = R"(
RUNSPEC
DIMENS
 10 10 10 /
GRID
START             -- 0
19 JUN 2007 /
SCHEDULE
DATES             -- 1
 10  OKT 2008 /
/
RPTRST
3 0 0 0 0 2
/
DATES             -- 2
 20  JAN 2010 /
/
DATES             -- 3
 20  JAN 2011 /
/
)";

    Opm::Parser parser;

    auto deck1 = parser.parseString( deckData1);
    RestartConfig rstConfig1( deck1, restart_info, {} );

    // Observe that this is true due to some undocumented guessing that
    // the initial restart file should be written if a RPTRST keyword is
    // found in the SOLUTION section, irrespective of the content of that
    // keyword.
    BOOST_CHECK(  rstConfig1.getWriteRestartFile( 0 ) );
    BOOST_CHECK( !rstConfig1.getWriteRestartFile( 1 ) );
    BOOST_CHECK(  rstConfig1.getWriteRestartFile( 2 ) );


    std::vector<std::string> expected = { "ACIP","BASIC", "BG","BO","BW","DEN","KRG", "KRO", "KRW", "NORST", "SFREQ", "VGAS", "VOIL", "VWAT"};
    const auto kw_list = fun::map( fst, rstConfig1.getRestartKeywords(2) );

    BOOST_CHECK_EQUAL_COLLECTIONS( expected.begin() ,expected.end(),
                                   kw_list.begin() , kw_list.end() );

    BOOST_CHECK_EQUAL( rstConfig1.getKeyword( "ALLPROPS" , 2 ) , 0);

    auto deck2 = parser.parseString( deckData2 );
    RestartConfig rstConfig2( deck2, restart_info, {} );

    const auto expected2 = { "BASIC", "FLOWS", "FREQ" };
    const auto kw_list2 = fun::map( fst, rstConfig2.getRestartKeywords( 2 ) );
    BOOST_CHECK_EQUAL_COLLECTIONS( expected2.begin(), expected2.end(),
                                   kw_list2.begin(), kw_list2.end() );

    BOOST_CHECK( !rstConfig2.getWriteRestartFile( 0 ) );
    BOOST_CHECK( !rstConfig2.getWriteRestartFile( 1 ) );
    BOOST_CHECK(  rstConfig2.getWriteRestartFile( 2 ) );
    BOOST_CHECK( !rstConfig2.getWriteRestartFile( 3 ) );

    auto deck3 = parser.parseString( deckData3 );
    RestartConfig rstConfig3( deck3, restart_info, {} );

    BOOST_CHECK( !rstConfig3.getWriteRestartFile( 0 ) );
    BOOST_CHECK( !rstConfig3.getWriteRestartFile( 1 ) );
    BOOST_CHECK(  rstConfig3.getWriteRestartFile( 2 ) );
    BOOST_CHECK( !rstConfig3.getWriteRestartFile( 3 ) );
}



BOOST_AUTO_TEST_CASE(RPTRST_FORMAT_ERROR) {

    const std::string deckData0 = R"(
RUNSPEC
DIMENS
 10 10 10 /
GRID
START             -- 0
19 JUN 2007 /
SOLUTION
RPTRST
 ACIP KRG KRO KRW NORST SFREQ=10 ALLPROPS/
SCHEDULE
DATES             -- 1
 10  OKT 2008 /
/
RPTRST
BASIC 1
/
DATES             -- 2
 20  JAN 2010 /
/
)";

    const std::string deckData1 = R"(
RUNSPEC
DIMENS
 10 10 10 /
GRID
START             -- 0
19 JUN 2007 /
SOLUTION
RPTRST
 ACIP KRG KRO KRW NORST SFREQ = 10 ALLPROPS/
SCHEDULE
DATES             -- 1
 10  OKT 2008 /
/
RPTRST
BASIC = 1
/
DATES             -- 2
 20  JAN 2010 /
/
)";

    const std::string deckData2 = R"(
RUNSPEC
DIMENS
 10 10 10 /
GRID
START             -- 0
19 JUN 2007 /
SCHEDULE
DATES             -- 1
 10  OKT 2008 /
/
RPTRST
BASIC = 3 FREQ = 2 FLOWS RUBBISH = 5
/
DATES             -- 2
 20  JAN 2010 /
/
DATES             -- 3
 20  JAN 2011 /
/
)";

    const std::string deckData3 = R"(
RUNSPEC
DIMENS
 10 10 10 /
GRID
START             -- 0
19 JUN 2007 /
SCHEDULE
DATES             -- 1
 10  OKT 2008 /
/
RPTRST
3 0 0 0 0 2
/
DATES             -- 2
 20  JAN 2010 /
/
DATES             -- 3
 20  JAN 2011 /
/
)";

    Opm::Parser parser;
    ParseContext ctx;
    ErrorGuard errors;

    auto deck0 = parser.parseString( deckData0, ctx, errors );
    auto deck1 = parser.parseString( deckData1, ctx, errors );
    ctx.update(ParseContext::RPT_UNKNOWN_MNEMONIC, InputError::IGNORE);
    ctx.update(ParseContext::RPT_MIXED_STYLE, InputError::THROW_EXCEPTION);
    BOOST_CHECK_THROW(RestartConfig(deck1, restart_info, {}, ctx, errors), OpmInputError);

    ctx.update(ParseContext::RPT_MIXED_STYLE, InputError::IGNORE);
    RestartConfig rstConfig1( deck1, restart_info, {}, ctx, errors );


    // The case "BASIC 1" - i.e. without '=' can not be salvaged; this should
    // give an exception whatever is the value of ParseContext::RPT_MIXED_STYLE:
    BOOST_CHECK_THROW(RestartConfig(deck0, restart_info, {}, ctx, errors), OpmInputError);


    // Observe that this is true due to some undocumented guessing that
    // the initial restart file should be written if a RPTRST keyword is
    // found in the SOLUTION section, irrespective of the content of that
    // keyword.
    BOOST_CHECK(  rstConfig1.getWriteRestartFile( 0 ) );
    BOOST_CHECK( !rstConfig1.getWriteRestartFile( 1 ) );
    BOOST_CHECK(  rstConfig1.getWriteRestartFile( 2 ) );


    std::vector<std::string> expected = { "ACIP","BASIC", "BG","BO","BW","DEN","KRG", "KRO", "KRW", "NORST", "SFREQ", "VGAS", "VOIL", "VWAT"};
    const auto kw_list = fun::map( fst, rstConfig1.getRestartKeywords(2) );

    BOOST_CHECK_EQUAL_COLLECTIONS( expected.begin() ,expected.end(),
                                   kw_list.begin() , kw_list.end() );

    BOOST_CHECK_EQUAL( rstConfig1.getKeyword( "ALLPROPS" , 2 ) , 0);

    auto deck2 = parser.parseString( deckData2, ctx, errors );

    ctx.update(ParseContext::RPT_UNKNOWN_MNEMONIC, InputError::THROW_EXCEPTION);
    BOOST_CHECK_THROW(RestartConfig(deck2, restart_info, {}, ctx, errors), OpmInputError);
    ctx.update(ParseContext::RPT_UNKNOWN_MNEMONIC, InputError::IGNORE);

    RestartConfig rstConfig2( deck2, restart_info, {}, ctx, errors );

    const auto expected2 = { "BASIC", "FLOWS", "FREQ" };
    const auto kw_list2 = fun::map( fst, rstConfig2.getRestartKeywords( 2 ) );
    BOOST_CHECK_EQUAL_COLLECTIONS( expected2.begin(), expected2.end(),
                                   kw_list2.begin(), kw_list2.end() );

    BOOST_CHECK( !rstConfig2.getWriteRestartFile( 0 ) );
    BOOST_CHECK( !rstConfig2.getWriteRestartFile( 1 ) );
    BOOST_CHECK(  rstConfig2.getWriteRestartFile( 2 ) );
    BOOST_CHECK( !rstConfig2.getWriteRestartFile( 3 ) );

    auto deck3 = parser.parseString( deckData3, ctx, errors );
    RestartConfig rstConfig3( deck3, restart_info, {}, ctx, errors );

    BOOST_CHECK( !rstConfig3.getWriteRestartFile( 0 ) );
    BOOST_CHECK( !rstConfig3.getWriteRestartFile( 1 ) );
    BOOST_CHECK(  rstConfig3.getWriteRestartFile( 2 ) );
    BOOST_CHECK( !rstConfig3.getWriteRestartFile( 3 ) );
}



BOOST_AUTO_TEST_CASE(RPTSCHED) {

    const std::string deckData1 = R"(
RUNSPEC
DIMENS
 10 10 10 /
GRID
START             -- 0
19 JUN 2007 /
SCHEDULE
DATES             -- 1
 10  OKT 2008 /
/
RPTSCHED
RESTART=1
/
DATES             -- 2
 20  JAN 2010 /
/
DATES             -- 3
 20  FEB 2010 /
/
RPTSCHED
RESTART=0
/
)";


    const std::string deckData2 = R"(
RUNSPEC
DIMENS
 10 10 10 /
GRID
START             -- 0
19 JUN 2007 /
SCHEDULE
DATES             -- 1
 10  OKT 2008 /
/
RPTSCHED
RESTART=3 FIP
/
DATES             -- 2
 20  JAN 2010 /
/
RPTSCHED
RESTART=4
/
DATES             -- 3
 20  FEB 2010 /
/
RPTSCHED
NOTHING RUBBISH
/
)";

    const std::string deckData3 = R"(
RUNSPEC
DIMENS
 10 10 10 /
GRID
START             -- 0
19 JUN 2007 /
SOLUTION
RPTSOL
  RESTART=4 /
SCHEDULE
DATES             -- 1
 10  OKT 2008 /
/
RPTRST
BASIC=3 FREQ=1 RUBBISH=5
/
DATES             -- 2
 20  JAN 2010 /
/
DATES             -- 3
 20  FEB 2010 /
/
RPTSCHED
0 0 0 0 0 0 0 0
/
)";

    Parser parser;

    auto deck1 = parser.parseString( deckData1 );
    RestartConfig rstConfig1( deck1, restart_info , {});

    BOOST_CHECK( !rstConfig1.getWriteRestartFile( 0 ) );
    BOOST_CHECK( !rstConfig1.getWriteRestartFile( 1 ) );
    BOOST_CHECK(  rstConfig1.getWriteRestartFile( 2 ) );
    BOOST_CHECK(  rstConfig1.getWriteRestartFile( 3 ) );


    auto deck2 = parser.parseString( deckData2 );
    RestartConfig rstConfig2( deck2, restart_info, {} );

    BOOST_CHECK( !rstConfig2.getWriteRestartFile( 0 ) );
    BOOST_CHECK( !rstConfig2.getWriteRestartFile( 1 ) );
    BOOST_CHECK(  rstConfig2.getWriteRestartFile( 2 ) );
    BOOST_CHECK(  rstConfig2.getWriteRestartFile( 3 ) );

    const auto expected2 = { "FIP", "RESTART" };
    const auto kw_list2 = fun::map( fst, rstConfig2.getRestartKeywords( 2 ) );
    BOOST_CHECK_EQUAL_COLLECTIONS( expected2.begin(), expected2.end(),
                                   kw_list2.begin(), kw_list2.end() );


    auto deck3 = parser.parseString( deckData3 );
    RestartConfig rstConfig3( deck3, restart_info , {});
    //Older ECLIPSE 100 data set may use integer controls instead of mnemonics
    BOOST_CHECK(  rstConfig3.getWriteRestartFile( 0 ) );
    BOOST_CHECK( !rstConfig3.getWriteRestartFile( 1 ) );
    BOOST_CHECK(  rstConfig3.getWriteRestartFile( 2 ) );
    BOOST_CHECK(  rstConfig3.getWriteRestartFile( 3 ) );

    std::vector<std::string> expected3 = { "BASIC", "FREQ" };
    const auto kw_list3 = fun::map( fst, rstConfig3.getRestartKeywords(2) );
    BOOST_CHECK_EQUAL_COLLECTIONS( expected3.begin() , expected3.end() , kw_list3.begin() , kw_list3.end() );
}


BOOST_AUTO_TEST_CASE(RPTSCHED_and_RPTRST) {
    const std::string deckData = R"(
RUNSPEC
DIMENS
 10 10 10 /
GRID
START             -- 0
19 JUN 2007 /
SCHEDULE
DATES             -- 1
 10  OKT 2008 /
/
RPTRST
BASIC=3 FREQ=3 BG BO
/
DATES             -- 2
 20  JAN 2010 /
/
DATES             -- 3
 20  FEB 2010 /
/
RPTSCHED
RESTART=1
/
)";


    Opm::Parser parser;

    auto deck = parser.parseString( deckData );
    RestartConfig rstConfig( deck, restart_info, {} );

    BOOST_CHECK( !rstConfig.getWriteRestartFile( 0 ) );
    BOOST_CHECK( !rstConfig.getWriteRestartFile( 1 ) );
    BOOST_CHECK( !rstConfig.getWriteRestartFile( 2 ) );
    BOOST_CHECK(  rstConfig.getWriteRestartFile( 3 ) );
}


BOOST_AUTO_TEST_CASE(NO_BASIC) {
    const std::string data = R"(
RUNSPEC
DIMENS
 10 10 10 /
GRID
START             -- 0
19 JUN 2007 /
SCHEDULE
DATES             -- 1
 10  OKT 2008 /
/
DATES             -- 2
 20  JAN 2010 /
/
DATES             -- 3
 20  FEB 2010 /
/
RPTSCHED
/
)";

    auto deck = Parser().parseString( data);
    RestartConfig ioConfig( deck, restart_info, {});

    for( size_t ts = 0; ts < 4; ++ts )
        BOOST_CHECK( !ioConfig.getWriteRestartFile( ts ) );
}

BOOST_AUTO_TEST_CASE(BASIC_EQ_1) {
    const std::string data = R"(
RUNSPEC
DIMENS
 10 10 10 /
GRID
START             -- 0
19 JUN 2007 /
SCHEDULE
DATES             -- 1
 10  OKT 2008 /
/
RPTRST
BASIC=3 FREQ=3
/
DATES             -- 2
 20  JAN 2010 /
/
DATES             -- 3
 20  FEB 2010 /
/
RPTSCHED
BASIC=1
/
)";

    auto deck = Parser().parseString( data);
    RestartConfig ioConfig( deck, restart_info, {});

    for( size_t ts = 0; ts < 3; ++ts )
        BOOST_CHECK( !ioConfig.getWriteRestartFile( ts ) );

    BOOST_CHECK( ioConfig.getWriteRestartFile( 3 ) );
}

BOOST_AUTO_TEST_CASE(BASIC_EQ_3) {
    const std::string data =  R"(
RUNSPEC
DIMENS
 10 10 10 /
GRID
START
 21 MAY 1981 /

SCHEDULE
RPTRST
BASIC=3 FREQ=3
/
DATES
 22 MAY 1981 /              -- timestep 1
 23 MAY 1981 /              -- timestep 2
 24 MAY 1981 /              -- timestep 3
 25 MAY 1981 /              -- timestep 4
 26 MAY 1981 /              -- timestep 5
 1 JAN 1982 /               -- timestep 6
 1 JAN 1982 13:55:44 /      -- timestep 7
 3 JAN 1982 14:56:45.123 /  -- timestep 8
 4 JAN 1982 14:56:45.123 /  -- timestep 9
 5 JAN 1982 14:56:45.123 /  -- timestep 10
 6 JAN 1982 14:56:45.123 /  -- timestep 11
/
)";

    auto deck = Parser().parseString( data);
    RestartConfig ioConfig(  deck, restart_info, {});

    const size_t freq = 3;

    /* BASIC=3, restart files are created every nth report time, n=3 */
    for( size_t ts = 1; ts < 12; ++ts )
        BOOST_CHECK_EQUAL( ts % freq == 0, ioConfig.getWriteRestartFile( ts ) );
}

BOOST_AUTO_TEST_CASE(BASIC_EQ_4) {
    const std::string data = R"(
RUNSPEC
DIMENS
 10 10 10 /
GRID
START
 21 MAY 1981 /

SCHEDULE
RPTRST
BASIC=4
/
DATES
 22 MAY 1981 /              -- timestep 1
 23 MAY 1981 /              -- timestep 2
 24 MAY 1981 /              -- timestep 3
 25 MAY 1981 /              -- timestep 4
 26 MAY 1981 /              -- timestep 5
 1 JAN 1982 /               -- timestep 6
 1 JAN 1982 13:55:44 /      -- timestep 7
 3 JAN 1982 14:56:45.123 /  -- timestep 8
 4 JAN 1982 14:56:45.123 /  -- timestep 9
 5 JAN 1982 14:56:45.123 /  -- timestep 10
 6 JAN 1982 14:56:45.123 /  -- timestep 11
 6 JAN 1983 14:56:45.123 /  -- timestep 12
/
)";

    auto deck = Parser().parseString( data);
    RestartConfig ioConfig( deck, restart_info, {});

    /* BASIC=4, restart file is written at the first report step of each year.
     */
    for( size_t ts : { 1, 2, 3, 4, 5, 7, 8, 9, 10, 11 } )
        BOOST_CHECK( !ioConfig.getWriteRestartFile( ts ) );

    for( size_t ts : { 6, 12 } )
        BOOST_CHECK( ioConfig.getWriteRestartFile( ts ) );
}

BOOST_AUTO_TEST_CASE(BASIC_EQ_4_FREQ_2) {
    const std::string data = R"(
RUNSPEC
DIMENS
 10 10 10 /
GRID
START
 21 MAY 1981 /

SCHEDULE
RPTRST
BASIC=4 FREQ=2
/
DATES
 22 MAY 1981 /
 23 MAY 1981 /
 24 MAY 1981 /
 23 MAY 1982 /
 24 MAY 1982 /
 24 MAY 1983 / -- write
 25 MAY 1984 /
 26 MAY 1984 /
 26 MAY 1985 / -- write
 27 MAY 1985 /
 1 JAN 1986 /
/
)";

    auto deck = Parser().parseString( data);
    RestartConfig ioConfig( deck, restart_info, {});

    /* BASIC=4, restart file is written at the first report step of each year.
     * Optionally, if the mnemonic FREQ is set >1 the restart is written only
     * every n'th year.
     *
     * FREQ=2
     */
    for( size_t ts : { 1, 2, 3, 4, 5, 7, 8, 10, 11  } )
        BOOST_CHECK( !ioConfig.getWriteRestartFile( ts ) );

    for( size_t ts : { 6, 9 } )
        BOOST_CHECK( ioConfig.getWriteRestartFile( ts ) );
}

BOOST_AUTO_TEST_CASE(BASIC_EQ_5) {
    const std::string data =  R"(
RUNSPEC
DIMENS
 10 10 10 /
GRID
START
 21 MAY 1981 /

SCHEDULE
RPTRST
BASIC=5 FREQ=2
/
DATES
 22 MAY 1981 /
 23 MAY 1981 /
 24 MAY 1981 /
  1 JUN 1981 /
  1 JUL 1981 / -- write
  1 JAN 1982 / -- write
  2 JAN 1982 /
  1 FEB 1982 /
  1 MAR 1982 / -- write
  1 APR 1983 / -- write
  2 JUN 1983 / -- write
/
)";

    auto deck = Parser().parseString( data);
    RestartConfig ioConfig( deck, restart_info,{});

    /* BASIC=5, restart file is written at the first report step of each month.
     */
    for( size_t ts : { 1, 2, 3, 4, 7, 8 } )
        BOOST_CHECK( !ioConfig.getWriteRestartFile( ts ) );

    for( size_t ts : { 5, 6, 9, 10, 11 } )
        BOOST_CHECK( ioConfig.getWriteRestartFile( ts ) );
}

BOOST_AUTO_TEST_CASE(BASIC_EQ_0) {
    const std::string data =  R"(
RUNSPEC
DIMENS
 10 10 10 /
GRID
START
 21 MAY 1981 /

SCHEDULE
RPTRST
BASIC=0 FREQ=2
/
DATES
 22 MAY 1981 /
 23 MAY 1981 /
 24 MAY 1981 /
  1 JUN 1981 /
  1 JUL 1981 /
  1 JAN 1982 /
  2 JAN 1982 /
  1 FEB 1982 /
  1 MAR 1982 /
  1 APR 1983 /
  2 JUN 1983 /
/
)";

    auto deck = Parser().parseString( data ) ;
    RestartConfig ioConfig( deck, restart_info, {});

    /* RESTART=0, no restart file is written
     */
    for( size_t ts = 0; ts < 11; ++ts )
        BOOST_CHECK( !ioConfig.getWriteRestartFile( ts ) );
}


BOOST_AUTO_TEST_CASE(RESTART_EQ_0) {
    const char* data =  R"(
RUNSPEC
DIMENS
 10 10 10 /
GRID
START
 21 MAY 1981 /

SCHEDULE
RPTSCHED
RESTART=0
/
DATES
 22 MAY 1981 /
 23 MAY 1981 /
 24 MAY 1981 /
  1 JUN 1981 /
  1 JUL 1981 /
  1 JAN 1982 /
  2 JAN 1982 /
  1 FEB 1982 /
  1 MAR 1982 /
  1 APR 1983 /
  2 JUN 1983 /
/
)";

    auto deck = Parser().parseString( data);
    RestartConfig ioConfig( deck, restart_info, {});

    /* RESTART=0, no restart file is written
     */
    for( size_t ts = 0; ts < 11; ++ts )
        BOOST_CHECK( !ioConfig.getWriteRestartFile( ts ) );
}

BOOST_AUTO_TEST_CASE(RESTART_BASIC_GT_2) {
    const std::string data =  R"(
RUNSPEC
DIMENS
 10 10 10 /
GRID
START
 21 MAY 1981 /

SCHEDULE
RPTRST
BASIC=4 FREQ=2
/
DATES
 22 MAY 1981 /
/
RPTSCHED // BASIC >2, ignore RPTSCHED RESTART
RESTART=3, FREQ=1
/
DATES
 23 MAY 1981 /
 24 MAY 1981 /
 23 MAY 1982 /
 24 MAY 1982 /
 24 MAY 1983 / -- write
 25 MAY 1984 /
 26 MAY 1984 /
 26 MAY 1985 / -- write
 27 MAY 1985 /
 1 JAN 1986 /
/
)";

    auto deck = Parser().parseString( data);
    RestartConfig ioConfig( deck, restart_info, {});

    for( size_t ts : { 1, 2, 3, 4, 5, 7, 8, 10, 11  } )
        BOOST_CHECK( !ioConfig.getWriteRestartFile( ts ) );

    for( size_t ts : { 6, 9 } )
        BOOST_CHECK( ioConfig.getWriteRestartFile( ts ) );
}

BOOST_AUTO_TEST_CASE(RESTART_BASIC_LEQ_2) {
    const std::string data = R"(
RUNSPEC
DIMENS
 10 10 10 /
GRID
START
 21 MAY 1981 /

SCHEDULE
RPTRST
BASIC=1"
/
DATES
 22 MAY 1981 /
/
RPTSCHED
RESTART=0
/
DATES
 23 MAY 1981 /
 24 MAY 1981 /
 23 MAY 1982 /
 24 MAY 1982 /
 24 MAY 1983 /
 25 MAY 1984 /
 26 MAY 1984 /
 26 MAY 1985 /
 27 MAY 1985 /
 1 JAN 1986 /
/
)";

    auto deck = Parser().parseString( data );
    RestartConfig ioConfig( deck, restart_info , {});

    BOOST_CHECK( ioConfig.getWriteRestartFile( 1 ) );
    for( size_t ts = 2; ts < 11; ++ts )
        BOOST_CHECK( !ioConfig.getWriteRestartFile( ts ) );
}

BOOST_AUTO_TEST_CASE(RESTART_SAVE) {
    const std::string data = R"(
RUNSPEC
DIMENS
 10 10 10 /
GRID
START
 21 MAY 1981 /

SCHEDULE
DATES
 22 MAY 1981 /
/
DATES
 23 MAY 1981 /
 24 MAY 1981 /
 23 MAY 1982 /
 24 MAY 1982 /
 24 MAY 1983 /
 25 MAY 1984 /
 26 MAY 1984 /
 26 MAY 1985 /
 27 MAY 1985 /
 1 JAN 1986 /
/
SAVE
TSTEP
 1 /
)";

    auto deck = Parser().parseString( data);
    RestartConfig ioConfig( deck, restart_info, {} );

    for( size_t ts = 1; ts < 11; ++ts )
        BOOST_CHECK( !ioConfig.getWriteRestartFile( ts ) );
    BOOST_CHECK( ioConfig.getWriteRestartFile( 12 ) );

}
