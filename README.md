# The-Journey-of-a-Signal
Twelve-hour introduction to electrical engineering course for astronomy/physics Olympiad medalists. Basic mathematical introduction to signals, control, amplifiers, signal processing, modulation and specially systems thinking. Includes slides and simple demo codes used during teaching.

# Goal
Give students a coherent mathematical picture of how a simple AM walkie-talkie works, using it as the unifying system to introduce major areas of electrical engineering.

# Contents
slides/ --- all lecture PDFs.

code/ --- minimal demo scripts (PID simulation, Schroeder reverb). They were built for teaching; expect rough edges.

# Video Lectures
The lecture videos can be found here (they're in Persian):

https://youtube.com/playlist?list=PLvJdr7DnUtcU72eMnktFv6xwv0y2NVGwX&si=BeVtiV1xGCV7wJic

# Structure
Day 1, "Systems Thinking"
- Complex numbers, LTI systems, convolution, Fourier transforms, positive and negative feedback, stability, PID control.
- Includes a python PID simulation code for a simple particle levitation control using charged surfaces. 

Day 2, "Systems By Circuits"
- Basic analog blocks, filtering, amplification, transistors and common-emitter amplifiers, conceptual talk about embedded systems

Day 3, "Preparing The Signal"
- Signals processing concepts, sampling theorem, phase vocoding and audio speeding-up, Schroeder's reverb algorithm, DSB-SC modulation,
  AM modulation, power efficiency of AM modulation, SSB modulation introduction, core EE curriculum courses review
- Includes a C++ Schroeder reverb algorithm
