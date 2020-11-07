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

/** Boolean variable controlling whether debug mode is enabled
 * during this run.
 */
extern bool debugEnabled;

/** Boolean variable enabling logging of computations made at the core of
 * the PID loop.
 */
extern bool coreLoggingEnabled;
