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
	printf("%s(%s, %s, %d): Unrecogized boolean value \"%s\".\n", __func__,
			section, key, def, result);
	return def;
}

#endif /* SKIPPY_CONFIG_H */

