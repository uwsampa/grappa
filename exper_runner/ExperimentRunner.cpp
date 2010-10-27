#include "ExperimentRunner.h"
#include <stdio.h>

ExperimentRunner::ExperimentRunner() {
	run_flag = false;
}

void ExperimentRunner::enable() {
	//TODO check if running and disallow
	run_flag = true;
}

void ExperimentRunner::disable() {
	//TODO check if running and disallow
	run_flag = false;
}

void ExperimentRunner::startIfEnabled() {
	if (run_flag) {
		printf("reqb");
		scanf("%s", dummy);
	}
}

void ExperimentRunner::stopIfEnabled() {
	if (run_flag) {
		printf("reqe");
		scanf("%s",dummy);
	}
}
