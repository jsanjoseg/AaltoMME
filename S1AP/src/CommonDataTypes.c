/* AaltoMME - Mobility Management Entity for LTE networks
 * Copyright (C) 2013 Vicent Ferrer Guash & Jesus Llorente Santos
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file   CommonDataTypes.c
 * @Author Vicent Ferrer
 * @date   April, 2013
 * @brief  S1AP Common data types definition
 *
 */

const char * CriticalityName [] = { "reject", "ignore", "notify" };
const char * MessageName [] = { "initiating_message", "successful_outcome", "unsuccessfull_outcome", "ext_not_implemented"};
const char * PresenceName [] = {"optional", "conditional", "mandatory"};
