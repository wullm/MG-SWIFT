/*******************************************************************************
 * This file is part of SWIFT.
 * Coypright (c) 2019 Loic Hausammann (loic.hausammann@epfl.ch)
 *               2019 Fabien Jeanquartier (fabien.jeanquartier@epfl.ch)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *******************************************************************************/
#ifndef SWIFT_GEAR_STAR_FORMATION_H
#define SWIFT_GEAR_STAR_FORMATION_H

/* Local includes */
#include "cooling.h"
#include "cosmology.h"
#include "engine.h"
#include "entropy_floor.h"
#include "error.h"
#include "hydro_properties.h"
#include "parser.h"
#include "part.h"
#include "physical_constants.h"
#include "random.h"
#include "star_formation_struct.h"
#include "units.h"

/**
 * @brief Calculate if the gas has the potential of becoming
 * a star.
 *
 * Use the star formation criterion given by eq. 3 in Revaz & Jablonka 2018.
 *
 * @param starform the star formation law properties to use.
 * @param p the gas particles.
 * @param xp the additional properties of the gas particles.
 * @param phys_const the physical constants in internal units.
 * @param cosmo the cosmological parameters and properties.
 * @param hydro_props The properties of the hydro scheme.
 * @param us The internal system of units.
 * @param cooling The cooling data struct.
 *
 */
INLINE static int star_formation_is_star_forming(
    struct part* restrict p, struct xpart* restrict xp,
    const struct star_formation* starform, const struct phys_const* phys_const,
    const struct cosmology* cosmo,
    const struct hydro_props* restrict hydro_props,
    const struct unit_system* restrict us,
    const struct cooling_function_data* restrict cooling,
    const struct entropy_floor_properties* restrict entropy_floor) {

  const float temperature =
      cooling_get_temperature(phys_const, hydro_props, us, cosmo, cooling, p, xp);

  const float temperature_max = starform->Max_temperature;

  /* Check the temperature criterion */
  if (T > T0) {
    return 0;
  }

  /* Get the required variables */
  const float G = phys_const->const_newton_G;
  const float kb = phys_const->const_boltzmann_k;
  const float mH = phys_const->const_proton_mass;

  const float sigma2 = xp->sf_data.sigma2;
  const int n_jeans_2_3 = starform->n_jeans_2_3;

  const float h = p->h;
  const float density = hydro_get_physical_density(p, cosmo);

  // TODO use GRACKLE */
  const float mu = hydro_props->mu_neutral;

  /* Compute the density criterion */
  const float coef = M_PI_4 / (G * n_jeans_2_3 * h * h);
  const float density_criterion = coef * (hydro_gamma * kb * T / (mu * mH) + sigma2);

  /* Check the density criterion */
  if (density > density_criterion) {
    return 1;
  } else {
    return 0;
  }
}

/**
 * @brief Compute the star-formation rate of a given particle.
 *
 * Nothing to do here. Everything is done in #star_formation_should_convert_to_star.
 *
 * @param p #part.
 * @param xp the #xpart.
 * @param starform the star formation law properties to use
 * @param phys_const the physical constants in internal units.
 * @param cosmo the cosmological parameters and properties.
 * @param dt_star The time-step of this particle.
 */
INLINE static void star_formation_compute_SFR(
    struct part* restrict p, struct xpart* restrict xp,
    const struct star_formation* starform, const struct phys_const* phys_const,
    const struct cosmology* cosmo, const double dt_star) {}

/**
 * @brief Decides whether a particle should be converted into a
 * star or not.

 * Compute the star formation rate from eq. 4 in Revaz & Jablonka 2012.
 *
 * @param p The #part.
 * @param xp The #xpart.
 * @param starform The properties of the star formation model.
 * @param e The #engine (for random numbers).
 * @param dt_star The time-step of this particle
 * @return 1 if a conversion should be done, 0 otherwise.
 */
INLINE static int star_formation_should_convert_to_star(
    struct part* p, struct xpart* xp, const struct star_formation* starform,
    const struct engine* e, const double dt_star) {

  /* Check that we are running a full time step */
  if (dt_star == 0.) {
    return 0;
  }

  /* Get a few variables */
  const float G = phys_const->const_newton_G;
  const float c_star = starform->star_formation_rate;
  const float density = hydro_get_physical_density(p, cosmo);

  /* Compute the probability */
  const float inv_free_fall_time = sqrtf(density * 32. * G / (3. * M_PI));
  const float prob = 1. - exp(-starform->star_formation_efficency * inv_free_fall_time * dt_star);

  /* Roll the dice... */
  const float random_number =
    random_unit_interval(p->id, e->ti_current, random_number_star_formation);

  if (random_number > prob) {
    /* No star for you */
    return 0;
  } else {
    /* You get a star, you get a star, everybody gets a star */
    return 1;
  }
}

/**
 * @brief Update the SF properties of a particle that is not star forming.
 *
 * @param p The #part.
 * @param xp The #xpart.
 * @param e The #engine.
 * @param starform The properties of the star formation model.
 * @param with_cosmology Are we running with cosmology switched on?
 */
INLINE static void star_formation_update_part_not_SFR(
    struct part* p, struct xpart* xp, const struct engine* e,
    const struct star_formation* starform, const int with_cosmology) {}

/**
 * @brief Copies the properties of the gas particle over to the
 * star particle.
 *
 * @param e The #engine
 * @param p the gas particles.
 * @param xp the additional properties of the gas particles.
 * @param sp the new created star particle with its properties.
 * @param starform the star formation law properties to use.
 * @param phys_const the physical constants in internal units.
 * @param cosmo the cosmological parameters and properties.
 * @param with_cosmology if we run with cosmology.
 */
INLINE static void star_formation_copy_properties(
    const struct part* p, const struct xpart* xp, struct spart* sp,
    const struct engine* e, const struct star_formation* starform,
    const struct cosmology* cosmo, const int with_cosmology,
    const struct phys_const* phys_const,
    const struct hydro_props* restrict hydro_props,
    const struct unit_system* restrict us,
    const struct cooling_function_data* restrict cooling) {

  /* Store the current mass */
  sp->mass = hydro_get_mass(p);

  /* Store either the birth_scale_factor or birth_time depending  */
  if (with_cosmology) {
    sp->birth_scale_factor = cosmo->a;
  } else {
    sp->birth_time = e->time;
  }

  // TODO copy only metals
  /* Store the chemistry struct in the star particle */
  sp->chemistry_data = p->chemistry_data;

  /* Store the tracers data */
  sp->tracers_data = xp->tracers_data;

  /* Store the birth density in the star particle */
  sp->birth_density = hydro_get_physical_density(p, cosmo);

  /* Store the birth temperature*/
  sp->birth_temperature =
      get_temperature(starform->phys_const, starform->hydro_props, starform->us,
                      cosmo, e->cooling_func, p, xp);
}

/**
 * @brief initialization of the star formation law
 *
 * @param parameter_file The parsed parameter file
 * @param phys_const Physical constants in internal units
 * @param us The current internal system of units
 * @param starform the star formation law properties to initialize
 *
 */
INLINE static void starformation_init_backend(
    struct swift_params* parameter_file, const struct phys_const* phys_const,
    const struct unit_system* us, const struct hydro_props* hydro_props,
    struct star_formation* starform) {

  // TODO move into pressure floor
  starform->n_jeans_2_3 =
      parser_get_param_float(parameter_file, "GEARStarFormation:NJeans");
  starform->n_jeans_2_3 = pow(starform->n_jeans_2_3, 2./3.);

  /* Star formation efficiency */
  starform->star_formation_efficiency = parser_get_param_double(
      parameter_file, "GEARStarFormation:star_formation_efficiency");

  /* Maximum temperature for star formation */
  starform->maximal_temperature =
      parser_get_param_double(parameter_file,
                              "GEARStarFormation:maximal_temperature");

  /* Apply unit change */
  starform->maximal_temperature *=
    units_cgs_conversion_factor(us, UNIT_CONV_TEMPERATURE);
}

/**
 * @brief Prints the used parameters of the star formation law
 *
 * @param starform the star formation law properties.
 */
INLINE static void starformation_print_backend(
    const struct star_formation* starform) {
  message("Star formation law is 'GEAR'");
}

/**
 * @brief Finishes the density calculation.
 *
 * @param p The particle to act upon
 * @param xp The extended particle data to act upon
 * @param cd The global star_formation information.
 * @param cosmo The current cosmological model.
 */
__attribute__((always_inline)) INLINE static void star_formation_end_density(
    struct part* restrict p, struct xpart* restrict xp,
    const struct star_formation* cd, const struct cosmology* cosmo) {

  // TODO move into pressure floor
  /* To finish the turbulence estimation we devide by the density */
  xp->sf_data.sigma2 /= pow_dimension(p->h) * hydro_get_physical_density(p, cosmo);
}

/**
 * @brief Sets all particle fields to sensible values when the #part has 0 ngbs.
 *
 * @param p The particle to act upon
 * @param xp The extended particle data to act upon
 * @param cd #star_formation containing star_formation informations.
 * @param cosmo The current cosmological model.
 */
__attribute__((always_inline)) INLINE static void
star_formation_part_has_no_neighbours(struct part* restrict p,
                                      struct xpart* restrict xp,
                                      const struct star_formation* cd,
                                      const struct cosmology* cosmo) {

  // TODO move into pressure floor
  /* If part has 0 neighbours, the estimation of turbulence is 0 */
  xp->sf_data.sigma2 = 0.f;
}

/**
 * @brief Sets the star_formation properties of the (x-)particles to a valid
 * start state.
 * @param phys_const The physical constant in internal units.
 * @param us The unit system.
 * @param cosmo The current cosmological model.
 * @param data The global star_formation information used for this run.
 * @param p Pointer to the particle data.
 */
__attribute__((always_inline)) INLINE static void
star_formation_first_init_part(const struct phys_const* restrict phys_const,
                               const struct unit_system* restrict us,
                               const struct cosmology* restrict cosmo,
                               const struct star_formation* data,
                               struct part* restrict p) {

  /* Nothing special here */
  star_formation_init_part(p, xp, data);
}

/**
 * @brief Sets the star_formation properties of the (x-)particles to a valid
 * start state.
 *
 * @param p Pointer to the particle data.
 * @param xp Pointer to extended particle data
 * @param data The global star_formation information.
 */
__attribute__((always_inline)) INLINE static void star_formation_init_part(
    struct part* restrict p, struct xpart* restrict xp,
    const struct star_formation* data) {
  xp->sf_data.sigma2 = 0.f;
}

#endif /* SWIFT_GEAR_STAR_FORMATION_H */
