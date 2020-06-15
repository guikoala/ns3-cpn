.. include:: replace.txt
.. highlight:: cpp

Clock per node implementation
----------------------------
In this document we present the main characteristics of the clock module that allows the implementation of different time notions within the ns-3 simulations. 
This implementation allows to run nodes with different notions of times based on a mapping function between the node clock (which is considered local time)
and global time (which is considered simulator time or true time). 
We propose a set of clear interfaces and classes that would allow researchers to future add different clock behavior models in ns-3.
We propose an open interface that:

   * allows to introduce independent local clocks, one per \textit{Node}

   * comes as an independent module and does not require any change in already-existing modules. After successful configuration of the simulation, all the \textit{\(Simulator::Schedule()\)} calls issued by already-existing ns-3 models will automatically be interpreted with respect to the local clock of the considered \textit{Node}.

   * provides an interface for designing more complex clock behavioral models.

   * provides an interface for updating the clock behavioral model during simulation time. For example, in an IEEE 1588 node, when a synchronization message is received, the synchronization function typically correct the frequency of the node's clock. In ns-3, this can be simulated by having an IEEE1588 \textit{Application} that triggers an update of the clock model (with the new frequency), using the interface that we provide. This interface is the basis for analyzing the interaction between local clocks and network events/mechanisms.


Model Description
*****************

The source code for the clock module lives in the directory ``src/clock``.

Design
======

Clock module is composed by 3 main classes which are LocalTimeSimulatorImpl, LocalClock and ClockModel.

LocalTimeSimulatorImpl substitute the default simulator implementation and it slightly diverts from it to include new functionalities that allows to simulate time notions in ns-3.
Additionally, a LocalClock object is aggregated to each Node. It manages the local clock and provides access to the clock model.

ClockModel
##########

ClockModel is the interface for the clock behavioral model. It provides the minimum set of functions that a clock model should implement. 
Any clock model developed should extend from this interface. In order to introduce the notion of local time in ns-3, the clock model must provide a 
mapping between the local time and the simulator time (also called global time, or true time). The relative time-function can capture clock non-idealities
in order to introduce realistic clocks. We are aware about the difficulty of creating realistic clock models.
However, the development of realistic clock models its out of the scope of this project. We present the interface that a clock model should use in order 
to be introduced in ns-3 

LocalClock
##########

A LocalClock object is aggregated to every Node using the ns-3 aggregation system. It represents the interface between the clock model and the node. 
It also tracks all the events that have been scheduled by this node with the purpose of rescheduling them if needed by an update of the clock model. 
Events are rescheduled when the ClockModel for this Node is updated using the SetClock() function (following an update on the clock's frequency for example).
If some events have been scheduled using the previous clock model, then the mapping from local time to global time is no more valid for these events,
so the LocalClock class reschedule them in accordance to the newly provided clock model.

To do so, events are retrieved from the event list of the LocalClock object and new execution times in the global time are calculated. 
New events are scheduled with the same \textit{EventImpl} (using a copy of the pointer).
Old events (which execution time does not correspond to the new clock) need to be removed from the scheduler. However, if events are cancelled using
Simulator::Cancel(), it would be impossible to execute the event implementation provided to the new event. Indeed, in ns-3, Simulator::Cancel() cancels 
the EventImpl, not the EventId. To avoid this problem, old events along with new events are pushed to the CancelEventsMap map of LocalTimeSimulatorImpl.
When Simulator::Schedule (const Time &delay, EventImpl *event) is called, the node object is retrieve from the NodeList using the current context of the 
simulator. Once the node object is retrieve, node->GetObject <LocalClock> () is called to obtain the local clock, if aggregated.
Using LocalClock object, main operations are done to translate the local delay into a global delay. After inserting the event in the simulator,
LocalClock->InsertEvent () is called in order to notify the node that the event is been scheduled, as explained in LocalClock section.

LocalTimeSimulatorImpl
######################

The main differences between LocalTimeSimulatorImpl and DefaultSimulatorImpl are: 

    *  When an event is scheduled using Simulator::Schedule() function, the delay is understood as being a local-time delay. 
    The delay is then translated into a global-time delay before being inserted into the scheduler (the scheduler only operates in the global-time domain.
    * When a clock model is updated, LocalTimeSimulatorImpl keeps track of the events that have been rescheduled, and will not execute the old events.

When Simulator::Schedule() is called, the Node object is retrieved from the NodeList using the current context of the simulator. 
When the context do not correspond to any node, delays are considered to be in global time.

When Simulator::ScheduleWithContext (uint32_t context, const Time &delay, EventImpl *event) no further actions are taken. the delay is considered 
to be in global time. This is because when a call to ScheduleWithContext happen, is due to a packet transmission within the channel. 
The delay in that call is normally the channel transmission time (As in CsmaNetDevice), which does not depend on the nodes clock. Because of that, 
this function remain unchanged in comparison with the DefaultSimulatorImpl function.

When Simulator::ScheduleWithContext() is called, the same actions as in DefaultSimulatorImpl are taken. 
The delay is understood as being already a global-time delay.

When Simulator::ScheduleNow() is called, a call to Schedule() function with local-time delay 0 is done.

One of the things to take into account is that Simulator::Now()  returns the current global time and not the local time.

Another particularity of this implementation resides in the CancelEventsMap map located in LocalTimeSimulatorImpl. When events are 
rescheduled, old events id (as key) with their corresponding new events (as value) are inserted in this map. As explained in the LocalClock 
section, we cannot use  Simulator::Cancel() to remove old events or their implementation would be lost.
In order not to execute events that should not be invoked (because the execution time attach to the event does not correspond to the new clock of
the node), ProcessOneEvent() function is slightly modifyied and checks the CancelEventsMap. 
If the EventId that is going to be executed is found as the map key, the event is skipped.

Other problem arises from the fact that the original event is never again valid, when rescheduling events. Any process (i.e Applications) that 
schedule events will never realize about the change of EventId due to the rescheduling. Therefore, there is a need to map between the original 
events and the reschedule events.
Scope and Limitations
=====================
Clock module attemp to introduce in ns-3 the concept of clocks with a clear interface and without any modification in the source code of ns-3.
However, it raises some drawback that need to be addressed while implementing. 
Simulator::Now () function, as mentionded before, returns the simulator time. In some of the modules this function is called with loggin purposes, which 
in general we desire to have them in global time. However, there exits some cases where the Simulator::Now () refers to a local time. In this cases,
maybe some changes should be done in the code in order to get the local time instead of the global time. 
Also, when Simulator::ScheduleWithContext() is called, the mayority of the cases the delay introduce is the propagation delay of the channel. 
However, there is one exception with point-to-point devices. In this case the delay accounts for the transmission delay and propagation delay. 
So, when applying clock module with point-to-point devices a way to decouple transmission delay from propagation delay must be done.
The function calling for the channel transmission should be done after the TrasnmitComplete event has been executed. 


Usage
*****

First of all an object ClockModel has to be created.::
   
   Ptr<PerfectClockModelImpl> clockImpl = CreateObject <PerfectClockModelImpl> ();

In this case the clock model used is the PerfectClockModelImpl which defines an affine function with an slope and offset between the global time and 
local time.::

   clockModel -> SetAttribute ("Frequency", DoubleValue (freq));
   clockModel -> SetAttribute ("Offset", TimeValue (init_offset));

After setting up the clock model it needs to be aggregated as a paramenter to the LocalClock object.::

   Ptr<LocalClock> clock = CreateObject<LocalClock> ();
   clock -> SetAttribute ("ClockModel", PointerValue (clockModelImpl));

Finally LocalClock object is aggregated to the node.::

   node->AggregateObject (clock);

Helpers
=======

This module does not use helpers.


Examples
========

The following examples have been written, which can be found in ``src/clock/examples/``:

* two-clocks-simple.cc. Two main nodes conected with point to point devices, where clocks in each node run independently as describe in the example.

Validation
**********

* Unit tests have been written to verify the internals of LocalTimeSimulatorImpl and LocalClock clasess using PerfectClockModelImpl. It can be found in ``src/test/clock-test.cc``
* The examples have been used to test Clock module with actual simulation scenarios. It can be found in ``src/examples``: