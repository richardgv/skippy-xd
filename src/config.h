/* Skippy - Seduces Kids Into Perversion
 *
 * Copyright (C) 2004 Hyriand <hyriand@thegraveyard.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef SKIPPY_CONFIG_H
#define SKIPPY_CONFIG_H

#include "skippy.h"
#include <strings.h>
#include <ctype.h>

dlist *config_load(const char *);
void config_free(dlist *);

const char *config_get(dlist *, const char *, const char *, const char *);

/**
 * @brief Get a boolean value from configuration.
 */
static inline bool
config_get_bool(dlist *config, const char *section, const char *key,
		bool def) {
	const char *strdef = (def ? "true": "false");
	const char *result = config_get(config, section, key, strdef);
	if (!strcasecmp("true", result))
		return true;
	if (!strcasecmp("false", result))
		return false;
	printfe("(%s, %s, %d): Unrecogized boolean value \"%s\".",
			section, key, def, result);
	return def;
}

/**
 * @brief Wrapper of config_get_bool().
 */
static inline void
config_get_bool_wrap(dlist *config, const char *section, const char *key,
		bool *tgt) {
	*tgt = config_get_bool(config, section, key, *tgt);
}

/**
 * @brief Get an int value from configuration.
 */
static inline int
config_get_int(dlist *config, const char *section, const char *key,
		int def, int min, int max) {
	const char *result = config_get(config, section, key, NULL);
	if (!result)
		return def;
	char *endptr = NULL;
	int iresult = strtol(result, &endptr, 0);
	if (!endptr || (*endptr && !isspace(*endptr))) {
		printfef("(%s, %s, %d): Value \"%s\" is not a valid integer.",
			section, key, def, result);
		return def;
	}
	if (iresult > max) {
		printfef("(%s, %s, %d): Value \"%s\" larger than maximum value %d.",
			section, key, def, result, max);
		return max;
	}
	if (iresult < min) {
		printfef("(%s, %s, %d): Value \"%s\" smaller than minimal value %d.",
			section, key, def, result, min);
		return min;
	}
	return iresult;
}

/**
 * @brief Wrapper of config_get_int().
 */
static inline void
config_get_int_wrap(dlist *config, const char *section, const char *key,
		int *tgt, int min, int max) {
	*tgt = config_get_int(config, section, key, *tgt, min, max);
}

/**
 * @brief Get a double value from configuration.
 */
static inline double
config_get_double(dlist *config, const char *section, const char *key,
		double def, double min, double max) {
	const char *result = config_get(config, section, key, NULL);
	if (!result)
		return def;
	char *endptr = NULL;
	double dresult = strtod(result, &endptr);
	if (!endptr || (*endptr && !isspace(*endptr))) {
		printfef("(%s, %s, %f): Value \"%s\" is not a valid floating-point number.",
			section, key, def, result);
		return def;
	}
	if (dresult > max) {
		printfef("(%s, %s, %f): Value \"%s\" larger than maximum value %f.",
			section, key, def, result, max);
		return max;
	}
	if (dresult < min) {
		printfef("(%s, %s, %f): Value \"%s\" smaller than minimal value %f.",
			section, key, def, result, min);
		return min;
	}
	return dresult;
}

/**
 * @brief Wrapper of config_get_double().
 */
static inline void
config_get_double_wrap(dlist *config, const char *section, const char *key,
		double *tgt, double min, double max) {
	*tgt = config_get_double(config, section, key, *tgt, min, max);
}

#endif /* SKIPPY_CONFIG_H */

