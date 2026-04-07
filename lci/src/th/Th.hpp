/*
 * 线程（Thread）是操作系统能够进行运算调度的
 * 最小单元。被包含在进程（Process）之中，
 * 是进程中实际运作单元。你可以将它理解为
 * 一条在代码中独立执行的路径。
 *
 * 本代码包含一个简易的多线程控制机制，需要共享的变量和
 * 函数声明为成员。线程主函数包含了流程控制。要使用需要
 * 重新定义其中的各个事件函数。
 *
 * ?: 已经将fsm改为通用的
 * 20250621(qing): 从派生改为持有
 */

#pragma once
#include "Fsm.hpp"
#include <iostream>
#include <memory>
#include <functional>
#include <chrono>
#include <thread>
namespace qing {
	/* 可控制线程类 */
	class Th {
	public:

		/* 无C原生数据结构成员，可以直接调用缺省的构造函数 */
		Th() = default;

		/* 因为持有线程类，不要轻举妄动 */
		Th(const Th&) = delete;

		/*
		 * 虚析构函数 以前通过作用域可关闭线程
		 *
		 * 如今会出现问题
		 */
		~Th() {
			//this->shut();
			//this->wait();
		}

		/* 检查状态，如果子类有特殊需求可以重写 */
		virtual Fsmc::Stat check() {
			return fsm.check();
		}

		/* 
		 * 变质——改变状态的方法
		 *
		 * 唤醒函数不可以创建临界区，否则会造成死锁
		 */
		[[noreturn]] virtual void wake() {
			fsm.set(Fsmc::Stat::START);
		}

		/* 静止 */
		[[noreturn]] virtual void shut() {
			{
				std::lock_guard<std::mutex> lk(mtx_th);
				fsm.set(Fsmc::Stat::SHUT);
				ready = true;
			}

			/* 全部唤醒 */
			cv.notify_all();
		}

		[[noreturn]] virtual void stop() {
			{
				std::lock_guard<std::mutex> lk(mtx_th);
				fsm.set(Fsmc::Stat::STOP);
				ready = true;
			}

			cv.notify_all();
		}

		[[noreturn]] virtual void run() {
			{
				std::lock_guard<std::mutex> lk(mtx_th);
				fsm.set(Fsmc::Stat::RUNNING);
				ready = true;
			}
			cv.notify_all();
		}

		/*
		 * ———：控制
		 *
		 * 尝试唤醒线路并等待它开始，据说该函数有问题
		 */
		inline void waitStart() {
			std::unique_lock<std::mutex> lk(mtx_th);
			wake(); /* 等待线程开始，wake()中不可以加锁，否则会造成死锁 */
			ready = false;
			cv.wait(lk, [this]{ return ready; });
		}

		/* 等待线程结束 */
		inline void waitClose() {
			if (th->joinable())
				th->join();
			th.reset();
		}

		/*
		 * 激活线程，申请线程对象，需要确保对象已经被构造了
		 * 所以不能放在构造函数内
		 */
		inline void Act() {
			if (th) return; /* 防止重复激活 */
			th = std::make_unique<std::thread> (&Th::main, this); /* 申请线程资源 */
		}

	protected:

		std::function<void()> StopEvent = [this] { fsm.suspend(); }; /* 静止事件 */
		std::function<void()> WakeEvent = [this] { this-> run(); }; /* 唤醒事件 */
		std::function<void()> LoopEvent = [this] {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}; /* 循环体 */
		std::function<void()> ClearEvent = [] {};


		/* 主函数 回字型循环 */
		[[noreturn]] virtual void main() {
			while (this->check() != Fsmc::Stat::SHUT) {

				/* 静止事件 */
				while (this->check() == Fsmc::Stat::STOP) {
					StopEvent();
				}

				// 唤醒事件
				if (this->check() == Fsmc::Stat::START) {
					WakeEvent();
				}

				// 进入运行态（通常要在WakeEvent()中设置）
				while (this->check() == Fsmc::Stat::RUNNING) {
					LoopEvent();
				}

				/* 清理事件 记得检查成员变量有没有获取资源 */
				ClearEvent();

			}
		}

	private:

		/* 提供线程安全 （之前从Fsm派生时父类中已有名为mtx的字段） */
		std::mutex mtx_th;

		/* 提供阻塞以等待线程的关闭 */
		std::condition_variable cv;

		/* 辅助条件变量的逻辑型 */
		bool ready = false;

		/* 持有状态机对象 */
		qing::Fsmc fsm;

		/* 持有线程指针 */
		std::unique_ptr<std::thread> th;

	};
}
