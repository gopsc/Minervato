/*
 * 有限状态佳（Finite State Machine）是一种用于建模系统行为的
 * 抽象数学模型。它的核心思想是，系统在任何给定时刻都处于一个
 * 有限、预定义的“状态”中，并且会根据接收到的输入或事件，从
 * 一个状态切换到另一个状态。
 */

#pragma once
#include <mutex>
#include <condition_variable>
namespace qing {
	/* Finite State Machine Common 每个数字代表一个状态 */
	class Fsmc {
	public:

		/* 状态枚举 可扩展 */
		enum class Stat: int {
			SHUT=-1, STOP=0, START=1, RUNNING=2
		};

		/* 检查当前状态 返回Stat */
		inline Stat check() {
			std::lock_guard<std::mutex> lk(mtx);
			return stat;
		}

		/* 设置状态 */
		inline void set(Stat stat) {
			{
				std::lock_guard<std::mutex> lk(mtx);
				this->stat = stat;
				ready = true;
			}

			/* 唤醒所有等待者 */
			cv.notify_all();
		}

	//protected:

		/* 阻塞线程。等待事件唤醒 */
		inline void suspend() {
			std::unique_lock<std::mutex> lk(mtx);
			ready = false;
			cv.wait(lk, [this] { return ready; });
		}
	
	private:

		/* 线程安全 同时条件变量也需要 */
		std::mutex mtx;

		/* 条件变量提供阻塞 */
		std::condition_variable cv;

		/* 辅助条件变量 */
		bool ready = false;

		/* 状态 */
		enum Stat stat = Stat::STOP;

	};
}
