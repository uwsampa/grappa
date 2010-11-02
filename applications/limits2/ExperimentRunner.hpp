#ifndef EXPER_RUNNER_H
#define EXPER_RUNNER_H

class ExperimentRunner {
	private:
		bool run_flag;
		char dummy[4];

	public:
		ExperimentRunner();
		void enable();
		void disable();

		void startIfEnabled();
		void stopIfEnabled();
};

#endif
