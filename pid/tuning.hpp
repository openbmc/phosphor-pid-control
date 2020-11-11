#pragma once

#include <string>

/** Boolean variable controlling whether tuning is enabled
 * during this run.
 */
extern bool tuningEnabled;

/** String variable with the folder for writing logs if logging is enabled.
 */
extern std::string loggingPath;
/** Boolean variable whether loggingPath is non-empty. */
extern bool loggingEnabled;

/** Boolean variable enabling negation of the worst-margin input into
 * the margin PID loop, useful for making the PID math easier:
 * greater negated margins represent hotter temperatures,
 * which should correspond to greater fan RPM speeds.
 * In other words, both numbers will move in the same direction.
 */
extern bool negateEnabled;
