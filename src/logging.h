#pragma once

#include <mstd/common.h>

void ccm_log_info(string source, string message);
void ccm_log_warning(string source, string message);
void ccm_log_error(string source, string message);

void ccm_log_info_f(string source, string message, ...);
void ccm_log_warning_f(string source, string message, ...);
void ccm_log_error_f(string source, string message, ...);