\# VisionLab Pro Engineering Instructions



\## Project Goal



VisionLab Pro is an architecture-level secondary development of the

original VisionLab project.



The target system is a real-time intelligent video analytics and

edge AI inference platform.



\## C++ Standard



Use C++20.



\## Architecture Rules



\- Prefer RAII.

\- Avoid global mutable state.

\- Never introduce detached std::thread.

\- Prefer std::jthread and std::stop\_token where appropriate.

\- Never run AI inference on the Qt GUI thread.

\- Core/domain modules must not depend on QML.

\- Detector implementations must not draw directly on cv::Mat.

\- Detectors must return structured Detection results.

\- Video capture, inference, tracking, analytics and rendering must remain decoupled.

\- Real-time pipeline communication must use bounded queues.

\- Frame queues must support DropOldest backpressure.

\- Inference engines must implement IInferenceEngine.

\- Detectors must implement IDetector.

\- Trackers must implement ITracker.

\- Analytics rules must implement IRule.

\- Detector extensions should use the plugin architecture.

\- Respect QObject thread affinity.

\- Avoid hard-coded absolute paths.

\- Do not modify unrelated files.

\- Keep the repository buildable after every logical task.



\## Ownership



\- Prefer value semantics.

\- Use std::unique\_ptr for exclusive ownership.

\- Use std::shared\_ptr only when ownership is genuinely shared.

\- Avoid raw owning pointers.

\- Explicitly analyze cv::Mat lifetime when passing frames between threads.



\## Development Workflow



Before implementing a non-trivial task:



1\. Inspect related code.

2\. Identify affected callers.

3\. Explain the proposed design.

4\. List files to modify.

5\. Identify ownership and concurrency risks.

6\. Implement only the requested task.

7\. Build.

8\. Run tests.

9\. Review the implementation.



Do not perform large repository-wide rewrites unless explicitly requested.



\## Task Completion Report



After every completed implementation task, append the result to the sole

living report on the user's Desktop:



`%USERPROFILE%\Desktop\VisionLab_Pro任务总结报告.docx`



Do not create a new .docx, a backup copy, or a separate markdown summary.

Use the helpers in tools/task_summary_report.py and follow

.cursor/rules/task-summary-report.mdc.

