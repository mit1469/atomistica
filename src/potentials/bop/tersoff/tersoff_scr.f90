!! ======================================================================
!! Atomistica - Interatomic potential library and molecular dynamics code
!! https://github.com/Atomistica/atomistica
!!
!! Copyright (2005-2015) Lars Pastewka <lars.pastewka@kit.edu> and others
!! See the AUTHORS file in the top-level Atomistica directory.
!!
!! This program is free software: you can redistribute it and/or modify
!! it under the terms of the GNU General Public License as published by
!! the Free Software Foundation, either version 2 of the License, or
!! (at your option) any later version.
!!
!! This program is distributed in the hope that it will be useful,
!! but WITHOUT ANY WARRANTY; without even the implied warranty of
!! MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
!! GNU General Public License for more details.
!!
!! You should have received a copy of the GNU General Public License
!! along with this program.  If not, see <http://www.gnu.org/licenses/>.
!! ======================================================================

! @meta
!    public:directory
!    classtype:tersoff_scr_t classname:TersoffScr interface:potentials
!    features:mask,per_at,per_bond
! @endmeta

!>
!! Screened Tersoff potential
!!
!! Screened Tersoff potential
!! See: Tersoff, Phys. Rev. Lett. 56, 632 (1986)
!! Tersoff, Phys. Rev. Lett. 61, 2879 (1988)
!! Tersoff, Phys. Rev. B 37, 6991 (1988)
!! Tersoff, Phys. Rev. B 38, 9902 (1988)
!! Tersoff, Phys. Rev. B 39, 5566 (1989)
!! Pastewka, Klemenz, Gumbsch, Moseler, arXiv:1301.2142
!<

#include "macros.inc"

module tersoff_scr
  use supplib

  use particles
  use neighbors

  implicit none

  private

#define SCREENING
#define CUTOFF_T             exp_cutoff_t

#define TERSOFF_MAX_REF      TERSOFF_SCR_MAX_REF
#define TERSOFF_MAX_EL       TERSOFF_SCR_MAX_EL
#define TERSOFF_MAX_PAIRS    TERSOFF_SCR_MAX_PAIRS

#define BOP_NAME             tersoff_scr
#define BOP_NAME_STR         "tersoff_scr"
#define BOP_STR              "TersoffScr"
#define BOP_KERNEL           tersoff_scr_kernel
#define BOP_TYPE             tersoff_scr_t
#define BOP_DB_TYPE          tersoff_scr_db_t

#define REGISTER_FUNC        tersoff_scr_register
#define INIT_FUNC            tersoff_scr_init
#define DEL_FUNC             tersoff_scr_del
#define GET_CUTOFF_FUNC      tersoff_scr_get_cutoff
#define BIND_TO_FUNC         tersoff_scr_bind_to
#define COMPUTE_FUNC         tersoff_scr_energy_and_forces

#include "tersoff_params.f90"

#include "tersoff_type.f90"

contains

#include "tersoff_module.f90"

#include "../bop_kernel.f90"

#include "tersoff_func.f90"

#include "tersoff_registry.f90"

endmodule tersoff_scr
