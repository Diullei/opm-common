/*
  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify it under the terms
  of the GNU General Public License as published by the Free Software
  Foundation, either version 3 of the License, or (at your option) any later
  version.

  OPM is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along with
  OPM.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <opm/parser/eclipse/EclipseState/Grid/SatfuncPropertyInitializers.hpp>

#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/parser/eclipse/EclipseState/Runspec.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/SgfnTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/SgofTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/SlgofTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/Sof2Table.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/Sof3Table.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/SwfnTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/SwofTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/Tabdims.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/TableContainer.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/TableManager.hpp>

#include <opm/parser/eclipse/Utility/Functional.hpp>

#include <algorithm>
#include <array>
#include <exception>
#include <iterator>
#include <stdexcept>
#include <string>

#include <stddef.h>

namespace {

    struct RawTableEndPoints
    {
        struct {
            std::vector<double> gas;
            std::vector<double> water;
        } connate;

        struct {
            std::vector<double> oil_in_gas;
            std::vector<double> oil_in_water;
            std::vector<double> gas;
            std::vector<double> water;
        } critical;

        struct {
            std::vector<double> gas;
            std::vector<double> water;
        } maximum;
    };

    /*
     * See the "Saturation Functions" chapter in the Eclipse Technical
     * Description; there are several alternative families of keywords which
     * can be used to enter relperm and capillary pressure tables.
     *
     * If SWOF and SGOF are specified in the deck it return I
     * If SWFN, SGFN and SOF3 are specified in the deck it return II
     * If keywords are missing or mixed, an error is given.
     */
    enum class SatfuncFamily { none = 0, I = 1, II = 2 };

    SatfuncFamily
    getSaturationFunctionFamily(const Opm::TableManager& tm,
                                const Opm::Phases&       ph)
    {
        const auto wat    = ph.active(::Opm::Phase::WATER);
        const auto oil    = ph.active(::Opm::Phase::OIL);
        const auto gas    = ph.active(::Opm::Phase::GAS);
        const auto threeP = gas && oil && wat;

        const auto family1 =       // SGOF/SLGOF and/or SWOF
            (gas && (tm.hasTables("SGOF") || tm.hasTables("SLGOF"))) ||
            (wat && tm.hasTables("SWOF"));

        const auto family2 =      // SGFN, SOF{2,3}, SWFN
            (gas && tm.hasTables("SGFN")) ||
            (oil && ((threeP && tm.hasTables("SOF3")) ||
                     tm.hasTables("SOF2"))) ||
            (wat && tm.hasTables("SWFN"));

        if (gas && tm.hasTables("SGOF") && tm.hasTables("SLGOF")) {
            throw std::invalid_argument("Both SGOF and SLGOF have been specified but these tables are mutually exclusive!");
        }

        if (family1 && family2) {
            throw std::invalid_argument("Saturation families should not be mixed\n"
                                        "Use either SGOF (or SLGOF) and/or SWOF or SGFN/SWFN and SOF2/SOF3");
        }

        if (!family1 && !family2) {
            throw std::invalid_argument("Saturations function must be specified using either "
                                        "family 1 or family 2 keywords\n"
                                        "Use either SGOF (or SLGOF) and/or SWOF or SGFN/SWFN and SOF2/SOF3");
        }

        if( family1 ) return SatfuncFamily::I;
        if( family2 ) return SatfuncFamily::II;
        return SatfuncFamily::none;
    }

    // enum class limit { min, max };

    std::vector<double>
    findMinWaterSaturation(const Opm::TableManager& tm,
                           const Opm::Phases&       ph)
    {
        const auto num_tables = tm.getTabdims().getNumSatTables();

        if (! ph.active(::Opm::Phase::WATER))
            return std::vector<double>(num_tables, 0.0);

        const auto& swofTables = tm.getSwofTables();
        const auto& swfnTables = tm.getSwfnTables();

        const auto famI = [&swofTables]( int i ) {
            return swofTables.getTable<Opm::SwofTable>( i ).getSwColumn().front();
        };

        const auto famII = [&swfnTables]( int i ) {
            return swfnTables.getTable<Opm::SwfnTable>( i ).getSwColumn().front();
        };

        switch( getSaturationFunctionFamily( tm, ph ) ) {
            case SatfuncFamily::I: return map( famI, Opm::fun::iota( num_tables ) );
            case SatfuncFamily::II: return map( famII, Opm::fun::iota( num_tables ) );
            default:
                throw std::domain_error("No valid saturation keyword family specified");
        }
    }

    std::vector<double>
    findMaxWaterSaturation(const Opm::TableManager& tm,
                           const Opm::Phases&       ph)
    {
        const auto num_tables = tm.getTabdims().getNumSatTables();

        if (! ph.active(::Opm::Phase::WATER))
            return std::vector<double>(num_tables, 0.0);

        const auto& swofTables = tm.getSwofTables();
        const auto& swfnTables = tm.getSwfnTables();

        const auto famI = [&swofTables]( int i ) {
            return swofTables.getTable<Opm::SwofTable>( i ).getSwColumn().back();
        };

        const auto famII = [&swfnTables]( int i ) {
            return swfnTables.getTable<Opm::SwfnTable>( i ).getSwColumn().back();
        };

        switch( getSaturationFunctionFamily( tm, ph ) ) {
            case SatfuncFamily::I: return map( famI, Opm::fun::iota( num_tables ) );
            case SatfuncFamily::II: return map( famII, Opm::fun::iota( num_tables ) );
            default:
                throw std::domain_error("No valid saturation keyword family specified");
        }
    }

    std::vector<double>
    findMinGasSaturation(const Opm::TableManager& tm,
                         const Opm::Phases&       ph)
    {
        const auto num_tables = tm.getTabdims().getNumSatTables();

        if (! ph.active(::Opm::Phase::GAS))
            return std::vector<double>(num_tables, 0.0);

        const auto& sgofTables  = tm.getSgofTables();
        const auto& slgofTables = tm.getSlgofTables();
        const auto& sgfnTables = tm.getSgfnTables();

        const auto famI_sgof = [&sgofTables]( int i ) {
            return sgofTables.getTable<Opm::SgofTable>( i ).getSgColumn().front();
        };

        const auto famI_slgof = [&slgofTables]( int i ) {
            return 1.0 - slgofTables.getTable<Opm::SlgofTable>( i ).getSlColumn().back();
        };

        const auto famII = [&sgfnTables]( int i ) {
            return sgfnTables.getTable<Opm::SgfnTable>( i ).getSgColumn().front();
        };

        switch( getSaturationFunctionFamily( tm, ph ) ) {
            case SatfuncFamily::I:
                if( sgofTables.empty() && slgofTables.empty() )
                    throw std::runtime_error( "Saturation keyword family I requires either sgof or slgof non-empty" );

                if( !sgofTables.empty() )
                    return Opm::fun::map( famI_sgof, Opm::fun::iota( num_tables ) );
                else
                    return Opm::fun::map( famI_slgof, Opm::fun::iota( num_tables ) );

            case SatfuncFamily::II:
                return Opm::fun::map( famII, Opm::fun::iota( num_tables ) );

            default:
                throw std::domain_error("No valid saturation keyword family specified");
        }
    }

    std::vector<double>
    findMaxGasSaturation(const Opm::TableManager& tm,
                         const Opm::Phases&       ph)
    {
        const auto num_tables = tm.getTabdims().getNumSatTables();

        if (! ph.active(::Opm::Phase::GAS))
            return std::vector<double>(num_tables, 0.0);

        const auto& sgofTables  = tm.getSgofTables();
        const auto& slgofTables = tm.getSlgofTables();
        const auto& sgfnTables = tm.getSgfnTables();

        const auto famI_sgof = [&sgofTables]( int i ) {
            return sgofTables.getTable<Opm::SgofTable>( i ).getSgColumn().back();
        };

        const auto famI_slgof = [&slgofTables]( int i ) {
            return 1.0 - slgofTables.getTable<Opm::SlgofTable>( i ).getSlColumn().front();
        };

        const auto famII = [&sgfnTables]( int i ) {
            return sgfnTables.getTable<Opm::SgfnTable>( i ).getSgColumn().back();
        };

        switch( getSaturationFunctionFamily( tm, ph ) ) {
            case SatfuncFamily::I:
                if( sgofTables.empty() && slgofTables.empty() )
                    throw std::runtime_error( "Saturation keyword family I requires either sgof or slgof non-empty" );

                if( !sgofTables.empty() )
                    return Opm::fun::map( famI_sgof, Opm::fun::iota( num_tables ) );
                else
                    return Opm::fun::map( famI_slgof, Opm::fun::iota( num_tables ) );

            case SatfuncFamily::II:
                return Opm::fun::map( famII, Opm::fun::iota( num_tables ) );

            default:
                throw std::domain_error("No valid saturation keyword family specified");
        }
    }

    /*
     * These functions have been ported from an older implementation to instead
     * use std::upper_bound and more from <algorithm> to make code -intent-
     * clearer. This also made some (maybe intentional) details easier to spot.
     * A short discussion:
     *
     * I don't know if not finding any element larger than 0.0 in the tables
     * was ever supposed to happen (or even possible), in which case the vector
     * elements remained at their initial value of 0.0. This behaviour has been
     * preserved, but is now explicit. The original code was also not clear if
     * it was possible to look up columns at index -1 (see critical_water for
     * an example), but the new version is explicit about this. Unfortuately
     * I'm not familiar enough with the maths or internal structure to make
     * more than a guess here, but most of this behaviour should be preserved.
     *
     */

    template <typename T>
    double critical_water( const T& table )
    {
        const auto& col = table.getKrwColumn();
        const auto end = col.begin() + table.numRows();
        const auto critical = std::upper_bound( col.begin(), end, 0.0 );
        const auto index = std::distance( col.begin(), critical );

        if( index == 0 || critical == end ) return 0.0;

        return table.getSwColumn()[ index - 1 ];
    }

    std::vector< double >
    findCriticalWater(const Opm::TableManager& tm,
                      const Opm::Phases&       ph)
    {
        const auto num_tables = tm.getTabdims().getNumSatTables();

        if (! ph.active(::Opm::Phase::WATER))
            return std::vector<double>(num_tables, 0.0);

        const auto& swofTables = tm.getSwofTables();
        const auto& swfnTables = tm.getSwfnTables();

        const auto famI = [&swofTables]( int i ) {
            return critical_water( swofTables.getTable<Opm::SwofTable>( i ) );
        };

        const auto famII = [&swfnTables]( int i ) {
            return critical_water( swfnTables.getTable<Opm::SwfnTable>( i ) );
        };

        switch( getSaturationFunctionFamily( tm, ph ) ) {
            case SatfuncFamily::I: return Opm::fun::map( famI, Opm::fun::iota( num_tables ) );
            case SatfuncFamily::II: return Opm::fun::map( famII, Opm::fun::iota( num_tables ) );
            default: throw std::domain_error("No valid saturation keyword family specified");
        }
    }

    template< typename T >
    double critical_gas( const T& table ) {
        const auto& col = table.getKrgColumn();
        const auto end = col.begin() + table.numRows();
        const auto critical = std::upper_bound( col.begin(), end, 0.0 );
        const auto index = std::distance( col.begin(), critical );

        if( index == 0 || critical == end ) return 0.0;

        return table.getSgColumn()[ index - 1 ];
    }

    double critical_gas( const Opm::SlgofTable& slgofTable ) {
        const auto& col = slgofTable.getKrgColumn();
        const auto critical = std::upper_bound( col.begin(), col.end(), 0.0 );
        const auto index = std::distance( col.begin(), critical );

        if( index == 0 || critical == col.end() ) return 0.0;

        return slgofTable.getSlColumn()[ index - 1 ];
    }

    std::vector<double>
    findCriticalGas(const Opm::TableManager& tm,
                    const Opm::Phases&       ph)
    {
        const auto num_tables = tm.getTabdims().getNumSatTables();

        if (! ph.active(::Opm::Phase::GAS))
            return std::vector<double>(num_tables, 0.0);

        const auto& sgfnTables = tm.getSgfnTables();
        const auto& sgofTables = tm.getSgofTables();
        const auto& slgofTables = tm.getSlgofTables();

        const auto famI_sgof = [&sgofTables]( int i ) {
            return critical_gas( sgofTables.getTable<Opm::SgofTable>( i ) );
        };

        const auto famI_slgof = [&slgofTables]( int i ) {
            return critical_gas( slgofTables.getTable<Opm::SlgofTable>( i ) );
        };

        const auto famII = [&sgfnTables]( int i ) {
            return critical_gas( sgfnTables.getTable<Opm::SgfnTable>( i ) );
        };

        switch( getSaturationFunctionFamily( tm, ph ) ) {
            case SatfuncFamily::I:
                if( sgofTables.empty() && slgofTables.empty() )
                    throw std::runtime_error( "Saturation keyword family I requires either sgof or slgof non-empty" );

                if( !sgofTables.empty() )
                    return Opm::fun::map( famI_sgof, Opm::fun::iota( num_tables ) );
                else
                    return Opm::fun::map( famI_slgof, Opm::fun::iota( num_tables ) );

            case SatfuncFamily::II:
                return Opm::fun::map( famII, Opm::fun::iota( num_tables ) );

            default:
                throw std::domain_error("No valid saturation keyword family specified");
        }
    }

    double critical_oil_water( const Opm::SwofTable& swofTable ) {
        const auto& col = swofTable.getKrowColumn();

        using reverse = std::reverse_iterator< decltype( col.begin() ) >;
        auto rbegin = reverse( col.begin() + swofTable.numRows() );
        auto rend = reverse( col.begin() );
        const auto critical = std::upper_bound( rbegin, rend, 0.0 );
        const auto index = std::distance( col.begin(), critical.base() - 1 );

        if( critical == rend ) return 0.0;

        return 1 - swofTable.getSwColumn()[ index + 1 ];
    }

    double critical_oil( const Opm::Sof2Table& sof2Table ) {
        const auto& col = sof2Table.getKroColumn();
        const auto critical = std::upper_bound( col.begin(), col.end(), 0.0 );
        const auto index = std::distance( col.begin(), critical );

        if( index == 0 || critical == col.end() ) return 0.0;

        return sof2Table.getSoColumn()[ index - 1 ];
    }

    double critical_oil( const Opm::Sof3Table& sof3Table, const Opm::TableColumn& col ) {
        const auto critical = std::upper_bound( col.begin(), col.end(), 0.0 );
        const auto index = std::distance( col.begin(), critical );

        if( index == 0 || critical == col.end() ) return 0.0;

        return sof3Table.getSoColumn()[ index - 1 ];
    }

    std::vector<double>
    findCriticalOilWater(const Opm::TableManager& tm,
                         const Opm::Phases&       ph)
    {
        const auto num_tables = tm.getTabdims().getNumSatTables();

        if (! ph.active(::Opm::Phase::OIL) ||
            ! ph.active(::Opm::Phase::WATER))
            return std::vector<double>(num_tables, 0.0);

        const auto& swofTables = tm.getSwofTables();
        const auto& sof2Tables = tm.getSof2Tables();
        const auto& sof3Tables = tm.getSof3Tables();

        const auto famI = [&swofTables]( int i ) {
            return critical_oil_water( swofTables.getTable<Opm::SwofTable>( i ) );
        };

        const auto famII_2p = [&sof2Tables]( int i ) {
            return critical_oil( sof2Tables.getTable<Opm::Sof2Table>( i ) );
        };

        const auto famII_3p = [&sof3Tables]( int i ) {
            const auto& tb = sof3Tables.getTable<Opm::Sof3Table>( i );
            return critical_oil( tb, tb.getKrowColumn() );
        };

        switch( getSaturationFunctionFamily( tm, ph ) ) {
            case SatfuncFamily::I: return Opm::fun::map( famI, Opm::fun::iota( num_tables ) );
            case SatfuncFamily::II:
                return ph.active(::Opm::Phase::GAS)
                    ? Opm::fun::map( famII_3p, Opm::fun::iota( num_tables ) )
                    : Opm::fun::map( famII_2p, Opm::fun::iota( num_tables ) );

            default: throw std::domain_error("No valid saturation keyword family specified");
        }
    }

    double critical_oil_gas( const Opm::SgofTable& sgofTable )
    {
        const auto& col = sgofTable.getKrogColumn();

        using reverse = std::reverse_iterator< decltype( col.begin() ) >;
        auto rbegin = reverse( col.begin() + sgofTable.numRows() );
        auto rend = reverse( col.begin() );
        const auto critical = std::upper_bound( rbegin, rend, 0.0 );
        if( critical == rend ) {
            return 0.0;
        }
        const auto index = std::distance( col.begin(), critical.base() - 1 );
        return 1.0 - sgofTable.getSgColumn()[ index + 1 ];
    }

    double critical_oil_gas( const Opm::SlgofTable& sgofTable )
    {
        const auto& col = sgofTable.getKrogColumn();
        const auto critical = std::upper_bound( col.begin(), col.end(), 0.0 );
        if (critical == col.end()) {
            return 0.0;
        }
        const auto index = std::distance( col.begin(), critical - 1);
        return sgofTable.getSlColumn()[ index ];
    }

    std::vector<double>
    findCriticalOilGas(const Opm::TableManager& tm,
                       const Opm::Phases&       ph)
    {
        const auto num_tables = tm.getTabdims().getNumSatTables();

        if (! ph.active(::Opm::Phase::OIL) ||
            ! ph.active(::Opm::Phase::GAS))
            return std::vector<double>(num_tables, 0.0);

        const auto& sgofTables = tm.getSgofTables();
        const auto& slgofTables = tm.getSlgofTables();
        const auto& sof2Tables = tm.getSof2Tables();
        const auto& sof3Tables = tm.getSof3Tables();

        const auto famI_sgof = [&sgofTables]( int i ) {
            return critical_oil_gas( sgofTables.getTable<Opm::SgofTable>( i ) );
        };

        const auto famI_slgof = [&slgofTables]( int i ) {
            return critical_oil_gas( slgofTables.getTable<Opm::SlgofTable>( i ) );
        };

        const auto famII_2p = [&sof2Tables]( int i ) {
            return critical_oil( sof2Tables.getTable<Opm::Sof2Table>( i ) );
        };

        const auto famII_3p = [&sof3Tables]( int i ) {
            const auto& tb = sof3Tables.getTable<Opm::Sof3Table>( i );
            return critical_oil( tb, tb.getKrogColumn() );
        };

        switch( getSaturationFunctionFamily( tm, ph ) ) {
            case SatfuncFamily::I:
                if( sgofTables.empty() && slgofTables.empty() )
                    throw std::runtime_error( "Saturation keyword family I requires either sgof or slgof non-empty" );

                if( !sgofTables.empty() )
                    return Opm::fun::map( famI_sgof, Opm::fun::iota( num_tables ) );
                else
                    return Opm::fun::map( famI_slgof, Opm::fun::iota( num_tables ) );

            case SatfuncFamily::II:
                return ph.active(::Opm::Phase::WATER)
                    ? Opm::fun::map( famII_3p, Opm::fun::iota( num_tables ) )
                    : Opm::fun::map( famII_2p, Opm::fun::iota( num_tables ) );

            default:
                throw std::domain_error("No valid saturation keyword family specified");
        }
    }

    std::vector<double>
    findMaxKrg(const Opm::TableManager& tm,
               const Opm::Phases&       ph)
    {
        const auto num_tables = tm.getTabdims().getNumSatTables();

        if (! ph.active(::Opm::Phase::GAS))
            return std::vector<double>(num_tables, 0.0);

        const auto& sgofTables = tm.getSgofTables();
        const auto& slgofTables = tm.getSlgofTables();
        const auto& sgfnTables = tm.getSgfnTables();

        const auto& famI_sgof = [&sgofTables]( int i ) {
            return sgofTables.getTable<Opm::SgofTable>( i ).getKrgColumn().back();
        };

        const auto& famI_slgof = [&slgofTables]( int i ) {
            return slgofTables.getTable<Opm::SlgofTable>( i ).getKrgColumn().front();
        };

        const auto& famII = [&sgfnTables]( int i ) {
            return sgfnTables.getTable<Opm::SgfnTable>( i ).getKrgColumn().back();
        };

        switch( getSaturationFunctionFamily( tm, ph ) ) {
            case SatfuncFamily::I:
                if( sgofTables.empty() && slgofTables.empty() )
                    throw std::runtime_error( "Saturation keyword family I requires either sgof or slgof non-empty" );
                if( !sgofTables.empty() )
                    return Opm::fun::map( famI_sgof, Opm::fun::iota( num_tables ) );
                else
                    return Opm::fun::map( famI_slgof, Opm::fun::iota( num_tables ) );
            case SatfuncFamily::II:
                return Opm::fun::map( famII, Opm::fun::iota( num_tables ) );
            default:
                throw std::domain_error("No valid saturation keyword family specified");
        }
    }

    std::vector<double>
    findKrgr(const Opm::TableManager& tm,
             const Opm::Phases&       ph,
             const RawTableEndPoints& ep)
    {
        const auto num_tables = tm.getTabdims().getNumSatTables();

        if (! ph.active(::Opm::Phase::GAS))
            return std::vector<double>(num_tables, 0.0);

        const auto& sgofTables = tm.getSgofTables();
        const auto& slgofTables = tm.getSlgofTables();
        const auto& sgfnTables = tm.getSgfnTables();

        const auto& famI_sgof = [&sgofTables]( int i ) {
            return sgofTables.getTable<Opm::SgofTable>( i ).getKrgColumn().front();
        };

        const auto& famI_slgof = [&slgofTables]( int i ) {
            return slgofTables.getTable<Opm::SlgofTable>( i ).getKrgColumn().back();
        };

        const auto& famII = [&sgfnTables]( int i ) {
            return sgfnTables.getTable<Opm::SgfnTable>( i ).getKrgColumn().back();
        };

        switch( getSaturationFunctionFamily( tm, ph ) ) {
            case SatfuncFamily::I:
                if( sgofTables.empty() && slgofTables.empty() )
                    throw std::runtime_error( "Saturation keyword family I requires either sgof or slgof non-empty" );
                if( !sgofTables.empty() )
                    return Opm::fun::map( famI_sgof, Opm::fun::iota( num_tables ) );
                else
                    return Opm::fun::map( famI_slgof, Opm::fun::iota( num_tables ) );
            case SatfuncFamily::II:
                return Opm::fun::map( famII, Opm::fun::iota( num_tables ) );
            default:
                throw std::domain_error("No valid saturation keyword family specified");
        }
    }

    std::vector<double>
    findKrwr(const Opm::TableManager& tm,
             const Opm::Phases&       ph,
             const RawTableEndPoints& ep)
    {
        const auto num_tables = tm.getTabdims().getNumSatTables();

        if (! ph.active(::Opm::Phase::WATER))
            return std::vector<double>(num_tables, 0.0);

        const auto& swofTables = tm.getSwofTables();
        const auto& swfnTables = tm.getSwfnTables();

        const auto& famI = [&swofTables]( int i ) {
            return swofTables.getTable<Opm::SwofTable>( i ).getKrwColumn().front();
        };

        const auto& famII = [&swfnTables]( int i ) {
            return swfnTables.getTable<Opm::SwfnTable>( i ).getKrwColumn().front();
        };

        switch( getSaturationFunctionFamily( tm, ph ) ) {
            case SatfuncFamily::I:
                return Opm::fun::map( famI, Opm::fun::iota( num_tables ) );
            case SatfuncFamily::II:
                return Opm::fun::map( famII, Opm::fun::iota( num_tables ) );
            default:
                throw std::domain_error("No valid saturation keyword family specified");
        }
    }

    std::vector<double>
    findKrorw(const Opm::TableManager& tm,
              const Opm::Phases&       ph,
              const RawTableEndPoints& ep)
    {
        const auto num_tables = tm.getTabdims().getNumSatTables();

        if (! ph.active(::Opm::Phase::OIL) ||
            ! ph.active(::Opm::Phase::WATER))
            return std::vector<double>(num_tables, 0.0);

        const auto& swofTables = tm.getSwofTables();
        const auto& sof2Tables = tm.getSof2Tables();
        const auto& sof3Tables = tm.getSof3Tables();

        const auto& famI = [&swofTables]( int i ) {
            const auto& swofTable = swofTables.getTable<Opm::SwofTable>( i );
            const auto& krwCol = swofTable.getKrwColumn();
            const auto crit = std::upper_bound( krwCol.begin(), krwCol.end(), 0.0 );
            const auto index = std::distance( krwCol.begin(), crit );

            if( crit == krwCol.end() ) return 0.0;

            return swofTable.getKrowColumn()[ index - 1 ];
        };

        const auto crit_water = findCriticalWater( tm, ph );
        const auto min_gas = findMinGasSaturation( tm, ph );
        const auto& famII_3p = [&sof3Tables,&crit_water,&min_gas]( int i ) {
            const double OilSatAtcritialWaterSat = 1.0 - crit_water[ i ] - min_gas[ i ];
            return sof3Tables.getTable<Opm::Sof3Table>( i )
                .evaluate("KROW", OilSatAtcritialWaterSat);
        };

        const auto famII_2p = [&sof2Tables,&crit_water,&min_gas]( int i ) {
            const double OilSatAtcritialWaterSat = 1.0 - crit_water[ i ] - min_gas[ i ];
            return sof2Tables.getTable<Opm::Sof2Table>( i )
                .evaluate("KRO", OilSatAtcritialWaterSat);
        };

        switch( getSaturationFunctionFamily( tm, ph ) ) {
            case SatfuncFamily::I:
                return Opm::fun::map( famI, Opm::fun::iota( num_tables ) );
            case SatfuncFamily::II:
                return ph.active(::Opm::Phase::GAS)
                    ? Opm::fun::map( famII_3p, Opm::fun::iota( num_tables ) )
                    : Opm::fun::map( famII_2p, Opm::fun::iota( num_tables ) );
            default:
                throw std::domain_error("No valid saturation keyword family specified");
        }
    }

    std::vector<double>
    findKrorg(const Opm::TableManager& tm,
              const Opm::Phases&       ph,
              const RawTableEndPoints& ep)
    {
        const auto num_tables = tm.getTabdims().getNumSatTables();

        if (! ph.active(::Opm::Phase::OIL) ||
            ! ph.active(::Opm::Phase::GAS))
            return std::vector<double>(num_tables, 0.0);

        const auto& sgofTables = tm.getSgofTables();
        const auto& slgofTables = tm.getSlgofTables();
        const auto& sof2Tables = tm.getSof2Tables();
        const auto& sof3Tables = tm.getSof3Tables();

        const auto& famI_sgof = [&sgofTables]( int i ) {
            const auto& sgofTable = sgofTables.getTable<Opm::SgofTable>( i );
            const auto& krgCol = sgofTable.getKrgColumn();
            const auto crit = std::upper_bound( krgCol.begin(), krgCol.end(), 0.0 );
            const auto index = std::distance( krgCol.begin(), crit );

            if( crit == krgCol.end() ) return 0.0;

            return sgofTable.getKrogColumn()[ index - 1 ];
        };

        const auto& famI_slgof = [&slgofTables]( int i ) {
            const auto& slgofTable = slgofTables.getTable<Opm::SlgofTable>( i );
            const auto& col = slgofTable.getKrgColumn();
            using reverse = std::reverse_iterator< decltype( col.begin() ) >;
            auto rbegin = reverse( col.begin() + slgofTable.numRows() );
            auto rend = reverse( col.begin() );
            const auto crit = std::upper_bound( rbegin, rend, 0.0 );
            // base() points to the next element in the forward order
            const auto index = std::distance( col.begin(), crit.base());

            if( crit == rend ) return 0.0;

            return slgofTable.getKrogColumn()[ index ];
        };

        const auto crit_gas = findCriticalGas( tm, ph );
        const auto min_water = findMinWaterSaturation( tm, ph );
        const auto& famII_3p = [&sof3Tables,&crit_gas,&min_water]( int i ) {
            const double OilSatAtcritialGasSat = 1.0 - crit_gas[ i ] - min_water[ i ];
            return sof3Tables.getTable<Opm::Sof3Table>( i )
                .evaluate("KROG", OilSatAtcritialGasSat);
        };

        const auto famII_2p = [&sof2Tables,&crit_gas,&min_water]( int i ) {
            const double OilSatAtcritialGasSat = 1.0 - crit_gas[ i ] - min_water[ i ];
            return sof2Tables.getTable<Opm::Sof2Table>( i )
                .evaluate("KRO", OilSatAtcritialGasSat);
        };

        switch( getSaturationFunctionFamily( tm, ph ) ) {
            case SatfuncFamily::I:
                if( sgofTables.empty() && slgofTables.empty() )
                    throw std::runtime_error( "Saturation keyword family I requires either sgof or slgof non-empty" );
                if( !sgofTables.empty() )
                    return Opm::fun::map( famI_sgof, Opm::fun::iota( num_tables ) );
                else
                    return Opm::fun::map( famI_slgof, Opm::fun::iota( num_tables ) );
            case SatfuncFamily::II:
                return ph.active(::Opm::Phase::WATER)
                    ? Opm::fun::map( famII_3p, Opm::fun::iota( num_tables ) )
                    : Opm::fun::map( famII_2p, Opm::fun::iota( num_tables ) );
            default:
                throw std::domain_error("No valid saturation keyword family specified");
        }
    }

    /* find the maximum output values of the water-oil system. the maximum oil
     * relperm is possibly wrong because we have two oil relperms in a threephase
     * system. the documentation is very ambiguos here, though: it says that the
     * oil relperm at the maximum oil saturation is scaled according to maximum
     * specified the KRO keyword. the first part of the statement points at
     * scaling the resultant threephase oil relperm, but then the gas saturation
     * is not taken into account which means that some twophase quantity must be
     * scaled.
     */
    std::vector<double>
    findMaxPcog(const Opm::TableManager& tm,
                const Opm::Phases&       ph)
    {
        const auto num_tables = tm.getTabdims().getNumSatTables();

        if (! ph.active(::Opm::Phase::OIL) ||
            ! ph.active(::Opm::Phase::GAS))
            return std::vector<double>(num_tables, 0.0);

        const auto& sgofTables = tm.getSgofTables();
        const auto& slgofTables = tm.getSlgofTables();
        const auto& sgfnTables = tm.getSgfnTables();

        const auto& famI_sgof = [&sgofTables]( int i ) {
            return sgofTables.getTable<Opm::SgofTable>( i ).getPcogColumn().back();
        };

        const auto& famI_slgof = [&slgofTables]( int i ) {
            return slgofTables.getTable<Opm::SlgofTable>( i ).getPcogColumn().front();
        };

        const auto& famII = [&sgfnTables]( int i ) {
            return sgfnTables.getTable<Opm::SgfnTable>( i ).getPcogColumn().back();
        };

        switch( getSaturationFunctionFamily( tm, ph ) ) {
            case SatfuncFamily::I:
                if( sgofTables.empty() && slgofTables.empty() )
                    throw std::runtime_error( "Saturation keyword family I requires either sgof or slgof non-empty" );
                if( !sgofTables.empty() )
                    return Opm::fun::map( famI_sgof, Opm::fun::iota( num_tables ) );
                else
                    return Opm::fun::map( famI_slgof, Opm::fun::iota( num_tables ) );
            case SatfuncFamily::II:
                return Opm::fun::map( famII, Opm::fun::iota( num_tables ) );
            default:
                throw std::domain_error("No valid saturation keyword family specified");
        }
    }

    std::vector<double>
    findMaxPcow(const Opm::TableManager& tm,
                const Opm::Phases&       ph)
    {
        const auto num_tables  = tm.getTabdims().getNumSatTables();

        if (! ph.active(::Opm::Phase::OIL) ||
            ! ph.active(::Opm::Phase::WATER))
            return std::vector<double>(num_tables, 0.0);

        const auto& swofTables = tm.getSwofTables();
        const auto& swfnTables = tm.getSwfnTables();

        const auto& famI = [&swofTables]( int i ) {
            return swofTables.getTable<Opm::SwofTable>( i ).getPcowColumn().front();
        };

        const auto& famII = [&swfnTables]( int i ) {
            return swfnTables.getTable<Opm::SwfnTable>( i ).getPcowColumn().front();
        };

        switch( getSaturationFunctionFamily( tm, ph ) ) {
            case SatfuncFamily::I:
                return Opm::fun::map( famI, Opm::fun::iota( num_tables ) );
            case SatfuncFamily::II:
                return Opm::fun::map( famII, Opm::fun::iota( num_tables ) );
            default:
                throw std::domain_error("No valid saturation keyword family specified");
        }
    }

    std::vector<double>
    findMaxKro(const Opm::TableManager& tm,
               const Opm::Phases&       ph)
    {
        const auto num_tables = tm.getTabdims().getNumSatTables();

        if (! ph.active(::Opm::Phase::OIL))
            return std::vector<double>(num_tables, 0.0);

        const auto wat = ph.active(::Opm::Phase::WATER);

        const auto& other_f1   = wat ? tm.getSwofTables() : tm.getSgofTables();
        const auto& sof2Tables = tm.getSof2Tables();
        const auto& sof3Tables = tm.getSof3Tables();

        const auto& famI = [&other_f1,wat]( int i ) {
            // In O/W/G runs this relies on Krog(Sg=0) == Krow(Sw=Swco),
            // meaning that the first entry in the KRO column--in each
            // saturation region--is equal in keywords SGOF and SWOF.
            return wat
                ? other_f1.getTable<Opm::SwofTable>( i ).getKrowColumn().front()
                : other_f1.getTable<Opm::SgofTable>( i ).getKrogColumn().front();
        };

        const auto& famII_2p = [&sof2Tables]( int i ) {
            return sof2Tables.getTable<Opm::Sof2Table>( i ).getKroColumn().back();
        };

        const auto& famII_3p = [&sof3Tables]( int i ) {
            return sof3Tables.getTable<Opm::Sof3Table>( i ).getKrowColumn().back();
        };

        switch( getSaturationFunctionFamily( tm, ph ) ) {
            case SatfuncFamily::I:
                return Opm::fun::map( famI, Opm::fun::iota( num_tables ) );
            case SatfuncFamily::II:
                return ph.active(::Opm::Phase::GAS) && ph.active(::Opm::Phase::WATER)
                    ? Opm::fun::map( famII_3p, Opm::fun::iota( num_tables ) )
                    : Opm::fun::map( famII_2p, Opm::fun::iota( num_tables ) );
            default:
                throw std::domain_error("No valid saturation keyword family specified");
        }
    }

    std::vector<double>
    findMaxKrw(const Opm::TableManager& tm,
               const Opm::Phases&       ph)
    {
        const auto num_tables = tm.getTabdims().getNumSatTables();

        if (! ph.active(::Opm::Phase::WATER))
            return std::vector<double>(num_tables, 0.0);

        const auto& swofTables = tm.getSwofTables();
        const auto& swfnTables = tm.getSwfnTables();

        const auto& famI = [&swofTables]( int i ) {
            return swofTables.getTable<Opm::SwofTable>( i ).getKrwColumn().back();
        };

        const auto& famII = [&swfnTables]( int i ) {
            return swfnTables.getTable<Opm::SwfnTable>( i ).getKrwColumn().back();
        };

        switch( getSaturationFunctionFamily( tm, ph ) ) {
            case SatfuncFamily::I:
                return Opm::fun::map( famI, Opm::fun::iota( num_tables ) );
            case SatfuncFamily::II:
                return Opm::fun::map( famII, Opm::fun::iota( num_tables ) );
            default:
                throw std::domain_error("No valid saturation keyword family specified");
        }
    }

    RawTableEndPoints
    getRawTableEndpoints(const Opm::TableManager& tm,
                         const Opm::Phases&       phases)
    {
        auto ep = RawTableEndPoints{};

        ep.connate.gas   = findMinGasSaturation(tm, phases);
        ep.connate.water = findMinWaterSaturation(tm, phases);

        ep.critical.oil_in_gas   = findCriticalOilGas(tm, phases);
        ep.critical.oil_in_water = findCriticalOilWater(tm, phases);
        ep.critical.gas          = findCriticalGas(tm, phases);
        ep.critical.water        = findCriticalWater(tm, phases);

        ep.maximum.gas   = findMaxGasSaturation(tm, phases);
        ep.maximum.water = findMaxWaterSaturation(tm, phases);

        return ep;
    }

    double selectValue(const Opm::TableContainer& depthTables,
                       int tableIdx,
                       const std::string& columnName,
                       double cellDepth,
                       double fallbackValue,
                       bool useOneMinusTableValue)
    {
        if( tableIdx < 0 ) return fallbackValue;

        const auto& table = depthTables.getTable( tableIdx );

        if( tableIdx >= int( depthTables.size() ) )
            throw std::invalid_argument("Not enough tables!");

        // evaluate the table at the cell depth
        const double value = table.evaluate( columnName, cellDepth );

        // a column can be fully defaulted. In this case, eval() returns a NaN
        // and we have to use the data from saturation tables
        if( !std::isfinite( value ) ) return fallbackValue;
        if( useOneMinusTableValue ) return 1 - value;
        return value;
    }

    void checkSatRegions(const std::size_t  cellIdx,
                         const int          satfunc,
                         const int          endfunc,
                         const std::string& satregname)
    {
        if ((satfunc < 0) || (endfunc < 0)) {
            throw std::invalid_argument {
                "Region Index Out of Bounds in Active Cell "
                + std::to_string(cellIdx) + ". " + satregname + " = "
                + std::to_string(satfunc + 1) + ", ENDNUM = "
                + std::to_string(endfunc + 1)
            };
        }
    }

    std::vector<double>
    satnumApply(size_t size,
                const std::string& columnName,
                const std::vector< double >& fallbackValues,
                const Opm::TableManager& tableManager,
                const std::vector<double>& cell_depth,
                const std::vector<int>& satnum_data,
                const std::vector<int>& endnum_data,
                bool useOneMinusTableValue)
    {
        std::vector< double > values( size, 0 );

        // Actually assign the defaults. If the ENPVD keyword was specified in the deck,
        // this currently cannot be done because we would need the Z-coordinate of the
        // cell and we would need to know how the simulator wants to interpolate between
        // sampling points. Both of these are outside the scope of opm-parser, so we just
        // assign a NaN in this case...
        const bool useEnptvd = tableManager.useEnptvd();
        const auto& enptvdTables = tableManager.getEnptvdTables();
        for( size_t cellIdx = 0; cellIdx < values.size(); cellIdx++ ) {
            int satTableIdx = satnum_data[cellIdx] - 1;
            int endNum = endnum_data[cellIdx] - 1;

            // Active cell better have {SAT,END}NUM > 0.
            checkSatRegions(cellIdx, satTableIdx, endNum, "SATNUM");

            values[cellIdx] = selectValue(enptvdTables,
                                          (useEnptvd && endNum >= 0) ? endNum : -1,
                                          columnName,
                                          cell_depth[cellIdx],
                                          fallbackValues[ satTableIdx ],
                                          useOneMinusTableValue);
        }

        return values;
    }

    std::vector<double>
    imbnumApply(size_t size,
                const std::string& columnName,
                const std::vector< double >& fallBackValues,
                const Opm::TableManager& tableManager,
                const std::vector<double>& cell_depth,
                const std::vector<int>& imbnum_data,
                const std::vector<int>& endnum_data,
                bool useOneMinusTableValue )
    {
        std::vector< double > values( size, 0 );

        // Actually assign the defaults. if the ENPVD keyword was specified in the deck,
        // this currently cannot be done because we would need the Z-coordinate of the
        // cell and we would need to know how the simulator wants to interpolate between
        // sampling points. Both of these are outside the scope of opm-parser, so we just
        // assign a NaN in this case...
        const bool useImptvd = tableManager.useImptvd();
        const Opm::TableContainer& imptvdTables = tableManager.getImptvdTables();
        for( size_t cellIdx = 0; cellIdx < values.size(); cellIdx++ ) {
            int imbTableIdx = imbnum_data[ cellIdx ] - 1;
            int endNum = endnum_data[ cellIdx ] - 1;

            // Active cell better have {IMB,END}NUM > 0.
            checkSatRegions(cellIdx, imbTableIdx, endNum, "IMBNUM");

            values[cellIdx] = selectValue(imptvdTables,
                                          (useImptvd && endNum >= 0) ? endNum : -1,
                                          columnName,
                                          cell_depth[cellIdx],
                                          fallBackValues[imbTableIdx],
                                          useOneMinusTableValue);
        }

        return values;
    }

    std::vector<double>
    SGLEndpoint(const Opm::TableManager&   tableManager,
                const Opm::Phases&         /* phases */,
                const RawTableEndPoints&   ep,
                const std::vector<double>& cell_depth,
                const std::vector<int>&    satnum,
                const std::vector<int>&    endnum)
    {
        return satnumApply(cell_depth.size(), "SGCO", ep.connate.gas,
                           tableManager, cell_depth, satnum, endnum, false);
    }

    std::vector<double>
    ISGLEndpoint(const Opm::TableManager&   tableManager,
                 const Opm::Phases&         /* phases */,
                 const RawTableEndPoints&   ep,
                 const std::vector<double>& cell_depth,
                 const std::vector<int>&    imbnum,
                 const std::vector<int>&    endnum)
    {
        return imbnumApply(cell_depth.size(), "SGCO", ep.connate.gas,
                           tableManager, cell_depth, imbnum, endnum, false);
    }

    std::vector<double>
    SGUEndpoint(const Opm::TableManager&   tableManager,
                const Opm::Phases&         /* phases */,
                const RawTableEndPoints&   ep,
                const std::vector<double>& cell_depth,
                const std::vector<int>&    satnum,
                const std::vector<int>&    endnum)
    {
        return satnumApply(cell_depth.size(), "SGMAX", ep.maximum.gas,
                           tableManager, cell_depth, satnum, endnum, false);
    }

    std::vector<double>
    ISGUEndpoint(const Opm::TableManager&   tableManager,
                 const Opm::Phases&         /* phases */,
                 const RawTableEndPoints&   ep,
                 const std::vector<double>& cell_depth,
                 const std::vector<int>&    imbnum,
                 const std::vector<int>&    endnum)
    {
        return imbnumApply(cell_depth.size(), "SGMAX", ep.maximum.gas,
                           tableManager, cell_depth, imbnum, endnum, false);
    }

    std::vector<double>
    SWLEndpoint(const Opm::TableManager&   tableManager,
                const Opm::Phases&         /* phases */,
                const RawTableEndPoints&   ep,
                const std::vector<double>& cell_depth,
                const std::vector<int>&    satnum,
                const std::vector<int>&    endnum)
    {
        return satnumApply(cell_depth.size(), "SWCO", ep.connate.water,
                           tableManager, cell_depth, satnum, endnum, false);
    }

    std::vector<double>
    ISWLEndpoint(const Opm::TableManager&   tableManager,
                 const Opm::Phases&         /* phases */,
                 const RawTableEndPoints&   ep,
                 const std::vector<double>& cell_depth,
                 const std::vector<int>&    imbnum,
                 const std::vector<int>&    endnum)
    {
        return imbnumApply(cell_depth.size(), "SWCO", ep.connate.water,
                           tableManager, cell_depth, imbnum, endnum, false);
    }

    std::vector<double>
    SWUEndpoint(const Opm::TableManager&   tableManager,
                const Opm::Phases&         /* phases */,
                const RawTableEndPoints&   ep,
                const std::vector<double>& cell_depth,
                const std::vector<int>&    satnum,
                const std::vector<int>&    endnum)
    {
        return satnumApply(cell_depth.size(), "SWMAX", ep.maximum.water,
                           tableManager, cell_depth, satnum, endnum, true);
    }

    std::vector<double>
    ISWUEndpoint(const Opm::TableManager&   tableManager,
                 const Opm::Phases&         /* phases */,
                 const RawTableEndPoints&   ep,
                 const std::vector<double>& cell_depth,
                 const std::vector<int>&    imbnum,
                 const std::vector<int>&    endnum)
    {
        return imbnumApply(cell_depth.size(), "SWMAX", ep.maximum.water,
                           tableManager, cell_depth, imbnum, endnum, true);
    }

    std::vector<double>
    SGCREndpoint(const Opm::TableManager&   tableManager,
                 const Opm::Phases&         /* phases */,
                 const RawTableEndPoints&   ep,
                 const std::vector<double>& cell_depth,
                 const std::vector<int>&    satnum,
                 const std::vector<int>&    endnum)
    {
        return satnumApply(cell_depth.size(), "SGCRIT", ep.critical.gas,
                           tableManager, cell_depth, satnum, endnum, false);
    }

    std::vector<double>
    ISGCREndpoint(const Opm::TableManager&   tableManager,
                  const Opm::Phases&         /* phases */,
                  const RawTableEndPoints&   ep,
                  const std::vector<double>& cell_depth,
                  const std::vector<int>&    imbnum,
                  const std::vector<int>&    endnum)
    {
        return imbnumApply(cell_depth.size(), "SGCRIT", ep.critical.gas,
                           tableManager, cell_depth, imbnum, endnum, false);
    }

    std::vector<double>
    SOWCREndpoint(const Opm::TableManager&   tableManager,
                  const Opm::Phases&         /* phases */,
                  const RawTableEndPoints&   ep,
                  const std::vector<double>& cell_depth,
                  const std::vector<int>&    satnum,
                  const std::vector<int>&    endnum)
    {
        return satnumApply(cell_depth.size(), "SOWCRIT", ep.critical.oil_in_water,
                           tableManager, cell_depth, satnum, endnum, false);
    }

    std::vector<double>
    ISOWCREndpoint(const Opm::TableManager&   tableManager,
                   const Opm::Phases&         /* phases */,
                   const RawTableEndPoints&   ep,
                   const std::vector<double>& cell_depth,
                   const std::vector<int>&    imbnum,
                   const std::vector<int>&    endnum)
    {
        return imbnumApply(cell_depth.size(), "SOWCRIT", ep.critical.oil_in_water,
                           tableManager, cell_depth, imbnum, endnum, false);
    }

    std::vector<double>
    SOGCREndpoint(const Opm::TableManager&   tableManager,
                  const Opm::Phases&         /* phases */,
                  const RawTableEndPoints&   ep,
                  const std::vector<double>& cell_depth,
                  const std::vector<int>&    satnum,
                  const std::vector<int>&    endnum)
    {
        return satnumApply(cell_depth.size(), "SOGCRIT", ep.critical.oil_in_gas,
                           tableManager, cell_depth, satnum, endnum, false);
    }

    std::vector<double>
    ISOGCREndpoint(const Opm::TableManager&   tableManager,
                   const Opm::Phases&         /* phases */,
                   const RawTableEndPoints&   ep,
                   const std::vector<double>& cell_depth,
                   const std::vector<int>&    imbnum,
                   const std::vector<int>&    endnum)
    {
        return imbnumApply(cell_depth.size(), "SOGCRIT", ep.critical.oil_in_gas,
                           tableManager, cell_depth, imbnum, endnum, false);
    }

    std::vector<double>
    SWCREndpoint(const Opm::TableManager&   tableManager,
                 const Opm::Phases&         /* phases */,
                 const RawTableEndPoints&   ep,
                 const std::vector<double>& cell_depth,
                 const std::vector<int>&    satnum,
                 const std::vector<int>&    endnum)
    {
        return satnumApply(cell_depth.size(), "SWCRIT", ep.critical.water,
                           tableManager, cell_depth, satnum, endnum, false);
    }

    std::vector<double>
    ISWCREndpoint(const Opm::TableManager&   tableManager,
                  const Opm::Phases&         /* phases */,
                  const RawTableEndPoints&   ep,
                  const std::vector<double>& cell_depth,
                  const std::vector<int>&    imbnum,
                  const std::vector<int>&    endnum)
    {
        return imbnumApply(cell_depth.size(), "SWCRIT", ep.critical.water,
                           tableManager, cell_depth, imbnum, endnum, false);
    }

    std::vector<double>
    PCWEndpoint(const Opm::TableManager&   tableManager,
                const Opm::Phases&         phases,
                const RawTableEndPoints&   /* ep */,
                const std::vector<double>& cell_depth,
                const std::vector<int>&    satnum,
                const std::vector<int>&    endnum)
    {
        const auto max_pcow = findMaxPcow(tableManager, phases);
        return satnumApply(cell_depth.size(), "PCW", max_pcow, tableManager,
                           cell_depth, satnum, endnum, false);
    }

    std::vector<double>
    IPCWEndpoint(const Opm::TableManager&   tableManager,
                 const Opm::Phases&         phases,
                 const RawTableEndPoints&   /* ep */,
                 const std::vector<double>& cell_depth,
                 const std::vector<int>&    imbnum,
                 const std::vector<int>&    endnum)
    {
        const auto max_pcow = findMaxPcow(tableManager, phases);
        return imbnumApply(cell_depth.size(), "IPCW", max_pcow, tableManager,
                           cell_depth, imbnum, endnum, false);
    }

    std::vector<double>
    PCGEndpoint(const Opm::TableManager&   tableManager,
                const Opm::Phases&         phases,
                const RawTableEndPoints&   /* ep */,
                const std::vector<double>& cell_depth,
                const std::vector<int>&    satnum,
                const std::vector<int>&    imbnum)
    {
        const auto max_pcog = findMaxPcog(tableManager, phases);
        return satnumApply(cell_depth.size(), "PCG", max_pcog, tableManager,
                           cell_depth, satnum, imbnum, false );
    }

    std::vector<double>
    IPCGEndpoint(const Opm::TableManager&   tableManager,
                 const Opm::Phases&         phases,
                 const RawTableEndPoints&   /* ep */,
                 const std::vector<double>& cell_depth,
                 const std::vector<int>&    imbnum,
                 const std::vector<int>&    endnum)
    {
        const auto max_pcog = findMaxPcog(tableManager, phases);
        return imbnumApply(cell_depth.size(), "IPCG", max_pcog, tableManager,
                           cell_depth, imbnum, endnum, false);
    }

    std::vector<double>
    KRWEndpoint(const Opm::TableManager&   tableManager,
                const Opm::Phases&         phases,
                const RawTableEndPoints&   /* ep */,
                const std::vector<double>& cell_depth,
                const std::vector<int>&    satnum,
                const std::vector<int>&    endnum)
    {
        const auto max_krw = findMaxKrw(tableManager, phases);
        return satnumApply(cell_depth.size(), "KRW", max_krw, tableManager,
                           cell_depth, satnum, endnum, false);
    }

    std::vector<double>
    IKRWEndpoint(const Opm::TableManager&   tableManager,
                 const Opm::Phases&         phases,
                 const RawTableEndPoints&   /* ep */,
                 const std::vector<double>& cell_depth,
                 const std::vector<int>&    imbnum,
                 const std::vector<int>&    endnum)
    {
        const auto max_krw = findMaxKrw(tableManager, phases);
        return imbnumApply(cell_depth.size(), "IKRW", max_krw, tableManager,
                           cell_depth, imbnum, endnum, false );
    }

    std::vector<double>
    KRWREndpoint(const Opm::TableManager&   tableManager,
                 const Opm::Phases&         phases,
                 const RawTableEndPoints&   ep,
                 const std::vector<double>& cell_depth,
                 const std::vector<int>&    satnum,
                 const std::vector<int>&    endnum)
    {
        const auto krwr = findKrwr(tableManager, phases, ep);
        return satnumApply(cell_depth.size(), "KRWR", krwr, tableManager,
                           cell_depth, satnum, endnum, false);
    }

    std::vector<double>
    IKRWREndpoint(const Opm::TableManager&   tableManager,
                  const Opm::Phases&         phases,
                  const RawTableEndPoints&   ep,
                  const std::vector<double>& cell_depth,
                  const std::vector<int>&    imbnum,
                  const std::vector<int>&    endnum)
    {
        const auto krwr = findKrwr(tableManager, phases, ep);
        return imbnumApply(cell_depth.size(), "IKRWR", krwr, tableManager,
                           cell_depth, imbnum, endnum, false);
    }

    std::vector<double>
    KROEndpoint(const Opm::TableManager&   tableManager,
                const Opm::Phases&         phases,
                const RawTableEndPoints&   /* ep */,
                const std::vector<double>& cell_depth,
                const std::vector<int>&    satnum,
                const std::vector<int>&    endnum)
    {
        const auto max_kro = findMaxKro(tableManager, phases);
        return satnumApply(cell_depth.size(), "KRO", max_kro, tableManager,
                           cell_depth, satnum, endnum, false);
    }

    std::vector<double>
    IKROEndpoint(const Opm::TableManager&   tableManager,
                 const Opm::Phases&         phases,
                 const RawTableEndPoints&   /* ep */,
                 const std::vector<double>& cell_depth,
                 const std::vector<int>&    imbnum,
                 const std::vector<int>&    endnum)
    {
        const auto max_kro = findMaxKro(tableManager, phases);
        return imbnumApply(cell_depth.size(), "IKRO", max_kro, tableManager,
                           cell_depth, imbnum, endnum, false);
    }

    std::vector<double>
    KRORWEndpoint(const Opm::TableManager&   tableManager,
                  const Opm::Phases&         phases,
                  const RawTableEndPoints&   ep,
                  const std::vector<double>& cell_depth,
                  const std::vector<int>&    satnum,
                  const std::vector<int>&    endnum)
    {
        const auto krorw = findKrorw(tableManager, phases, ep);
        return satnumApply(cell_depth.size(), "KRORW", krorw, tableManager,
                           cell_depth, satnum, endnum, false);
    }

    std::vector<double>
    IKRORWEndpoint(const Opm::TableManager&   tableManager,
                   const Opm::Phases&         phases,
                   const RawTableEndPoints&   ep,
                   const std::vector<double>& cell_depth,
                   const std::vector<int>&    imbnum,
                   const std::vector<int>&    endnum)
    {
        const auto krorw = findKrorw(tableManager, phases, ep);
        return imbnumApply(cell_depth.size(), "IKRORW", krorw, tableManager,
                           cell_depth, imbnum, endnum, false);
    }

    std::vector<double>
    KRORGEndpoint(const Opm::TableManager&   tableManager,
                  const Opm::Phases&         phases,
                  const RawTableEndPoints&   ep,
                  const std::vector<double>& cell_depth,
                  const std::vector<int>&    satnum,
                  const std::vector<int>&    endnum)
    {
        const auto krorg = findKrorg(tableManager, phases, ep);
        return satnumApply(cell_depth.size(), "KRORG", krorg, tableManager,
                           cell_depth, satnum, endnum, false);
    }

    std::vector<double>
    IKRORGEndpoint(const Opm::TableManager&   tableManager,
                   const Opm::Phases&         phases,
                   const RawTableEndPoints&   ep,
                   const std::vector<double>& cell_depth,
                   const std::vector<int>&    imbnum,
                   const std::vector<int>&    endnum)
    {
        const auto krorg = findKrorg(tableManager, phases, ep);
        return imbnumApply(cell_depth.size(), "IKRORG", krorg, tableManager,
                           cell_depth, imbnum, endnum, false);
    }

    std::vector<double>
    KRGEndpoint(const Opm::TableManager&   tableManager,
                const Opm::Phases&         phases,
                const RawTableEndPoints&   /* ep */,
                const std::vector<double>& cell_depth,
                const std::vector<int>&    satnum,
                const std::vector<int>&    endnum)
    {
        const auto max_krg = findMaxKrg(tableManager, phases);
        return satnumApply(cell_depth.size(), "KRG", max_krg, tableManager,
                           cell_depth, satnum, endnum, false);
    }

    std::vector<double>
    IKRGEndpoint(const Opm::TableManager&   tableManager,
                 const Opm::Phases&         phases,
                 const RawTableEndPoints&   /* ep */,
                 const std::vector<double>& cell_depth,
                 const std::vector<int>&    imbnum,
                 const std::vector<int>&    endnum)
    {
        const auto max_krg = findMaxKrg(tableManager, phases);
        return imbnumApply(cell_depth.size(), "IKRG", max_krg, tableManager,
                           cell_depth, imbnum, endnum, false);
    }

    std::vector<double>
    KRGREndpoint(const Opm::TableManager&   tableManager,
                 const Opm::Phases&         phases,
                 const RawTableEndPoints&   ep,
                 const std::vector<double>& cell_depth,
                 const std::vector<int>&    satnum,
                 const std::vector<int>&    endnum)
    {
        const auto krgr = findKrgr(tableManager, phases, ep);
        return satnumApply(cell_depth.size(), "KRGR", krgr, tableManager,
                           cell_depth, satnum, endnum, false);
    }

    std::vector<double>
    IKRGREndpoint(const Opm::TableManager&   tableManager,
                  const Opm::Phases&         phases,
                  const RawTableEndPoints&   ep,
                  const std::vector<double>& cell_depth,
                  const std::vector<int>&    imbnum,
                  const std::vector<int>&    endnum)
    {
        const auto krgr = findKrgr(tableManager, phases, ep);
        return imbnumApply(cell_depth.size(), "IKRGR", krgr, tableManager,
                           cell_depth, imbnum, endnum, false);
    }
} // namespace Anonymous

std::vector<double>
Opm::satfunc::init(const std::string&         keyword,
                   const TableManager&        tables,
                   const Phases&              phases,
                   const std::vector<double>& cell_depth,
                   const std::vector<int>&    num,
                   const std::vector<int>&    endnum)
{
    using func_type = decltype(&IKRGEndpoint);

#define dirfunc(base, func) \
    {base, func}, \
    {base "X", func}, {base "X-", func},  \
    {base "Y", func}, {base "Y-", func},  \
    {base "Z", func}, {base "Z-", func}

    static const std::map<std::string, func_type> func_table = {
        // Drainage                      Imbibition
        {"SGLPC", SGLEndpoint},          {"ISGLPC", ISGLEndpoint},
        {"SWLPC", SWLEndpoint},          {"ISWLPC", ISWLEndpoint},

        dirfunc("SGL",   SGLEndpoint),   dirfunc("ISGL",   ISGLEndpoint),
        dirfunc("SGU",   SGUEndpoint),   dirfunc("ISGU",   ISGUEndpoint),
        dirfunc("SWL",   SWLEndpoint),   dirfunc("ISWL",   ISWLEndpoint),
        dirfunc("SWU",   SWUEndpoint),   dirfunc("ISWU",   ISWUEndpoint),

        dirfunc("SGCR",  SGCREndpoint),  dirfunc("ISGCR",  ISGCREndpoint),
        dirfunc("SOGCR", SOGCREndpoint), dirfunc("ISOGCR", ISOGCREndpoint),
        dirfunc("SOWCR", SOWCREndpoint), dirfunc("ISOWCR", ISOWCREndpoint),
        dirfunc("SWCR",  SWCREndpoint),  dirfunc("ISWCR",  ISWCREndpoint),

        dirfunc("PCG",   PCGEndpoint),   dirfunc("IPCG",   IPCGEndpoint),
        dirfunc("PCW",   PCWEndpoint),   dirfunc("IPCW",   IPCWEndpoint),

        dirfunc("KRG",   KRGEndpoint),   dirfunc("IKRG",   IKRGEndpoint),
        dirfunc("KRGR",  KRGREndpoint),  dirfunc("IKRGR",  IKRGREndpoint),
        dirfunc("KRO",   KROEndpoint),   dirfunc("IKRO",   IKROEndpoint),
        dirfunc("KRORW", KRORWEndpoint), dirfunc("IKRORW", IKRORWEndpoint),
        dirfunc("KRORG", KRORGEndpoint), dirfunc("IKRORG", IKRORGEndpoint),
        dirfunc("KRW",   KRWEndpoint),   dirfunc("IKRW",   IKRWEndpoint),
        dirfunc("KRWR",  KRWREndpoint),  dirfunc("IKRWR",  IKRWREndpoint),
    };

#undef dirfunc

    auto func = func_table.find(keyword);
    if (func == func_table.end())
        throw std::invalid_argument {
            "Unsupported saturation function scaling '"
            + keyword + '\''
        };

    const auto ep = getRawTableEndpoints(tables, phases);

    return func->second(tables, phases, ep, cell_depth, num, endnum);
}
