#if defined _OutputInfo_Included
 #endinput
#endif
#define _OutputInfo_Included

#define EVENT_FIRE_ALWAYS -1

/**
 * Gets the number of actions for given output
 * Note that this can change if an action has been deleted due to being used more than TimesToFire
 *
 * @param entity		Entity to use
 * @param output		The name of the output (e.g. m_OnTrigger)
 *
 * @return				Number of actions for that entity output
 */
native int GetOutputActionCount(int entity, const char[] output);

/**
 * Gets the name of the entity(s) to cause the action in
 * !activator = entity that activated output, !caller = entity that called output
 *
 * @param entity		Entity to use
 * @param output		The name of the output (e.g. m_OnTrigger)
 * @param index			The index of the action to use
 * @param target		Output string buffer
 * @param maxlen		Max length of output string buffer
 
 * @return				True on success, false otherwise
 */
native bool GetOutputActionTarget(int entity, const char[] output, int index, char[] target, int maxlen);

/**
 * Sets the name of the entity(s) to cause the action in
 * !activator = entity that activated output, !caller = entity that called output
 *
 * @param entity		Entity to use
 * @param output		The name of the output (e.g. m_OnTrigger)
 * @param index			The index of the action to use
 * @param target		Name of target
 
 * @return				True on success, false otherwise
 */
native bool SetOutputActionTarget(int entity, const char[] output, int index, const char[] target);

/**
 * Gets the name of the action to fire
 *
 * @param entity		Entity to use
 * @param output		The name of the output (e.g. m_OnTrigger)
 * @param index			The index of the action to use
 * @param targetinput	Output string buffer
 * @param maxlen		Max length of output string buffer
 
 * @return				True on success, false otherwise
 */
native bool GetOutputActionTargetInput(int entity, const char[] output, int index, char[] targetinput, int maxlen);

/**
 * Sets the name of the action to fire
 *
 * @param entity		Entity to use
 * @param output		The name of the output (e.g. m_OnTrigger)
 * @param index			The index of the action to use
 * @param targetinput	Name of action
 
 * @return				True on success, false otherwise
 */
native bool SetOutputActionTargetInput(int entity, const char[] output, int index, const char[] targetinput);

/**
 * Gets the parameter to send, 0 if none
 *
 * @param entity		Entity to use
 * @param output		The name of the output (e.g. m_OnTrigger)
 * @param index			The index of the action to use
 * @param parameter		Output string buffer
 * @param maxlen		Max length of output string buffer
 
 * @return				True on success, false otherwise
 */
native bool GetOutputActionParameter(int entity, const char[] output, int index, char[] parameter, int maxlen);

/**
 * Sets the parameter to send, 0 if none
 *
 * @param entity		Entity to use
 * @param output		The name of the output (e.g. m_OnTrigger)
 * @param index			The index of the action to use
 * @param parameter		Parameter to send
 
 * @return				True on success, false otherwise
 */
native bool SetOutputActionParameter(int entity, const char[] output, int index, const char[] parameter);

/**
 * Gets the number of seconds to wait before firing the action
 *
 * @param entity		Entity to use
 * @param output		The name of the output (e.g. m_OnTrigger)
 * @param index			The index of the action to use
 
 * @return				Delay in seconds, or -1.0 on failure
 */
native float GetOutputActionDelay(int entity, const char[] output, int index);

/**
 * Sets the number of seconds to wait before firing the action
 *
 * @param entity		Entity to use
 * @param output		The name of the output (e.g. m_OnTrigger)
 * @param index			The index of the action to use
 * @param value			Delay in seconds

 * @return				True on success, false otherwise
 */
native bool SetOutputActionDelay(int entity, const char[] output, int index, float value);

/**
 * Gets the amount of times left for an action to fire
 * When this counter reaches 0, the action is removed from the outputs action list
 *
 * @param entity		Entity to use
 * @param output		The name of the output (e.g. m_OnTrigger)
 * @param index			The index of the action to use
 
 * @return				How many times to fire, or EVENT_FIRE_ALWAYS if it fires indefinitely
 */
native int GetOutputActionTimesToFire(int entity, const char[] output, int index);

/**
 * Sets the amount of times left for an action to fire
 * If set to 0, this action is deleted from the action list
 *
 * @param entity		Entity to use
 * @param output		The name of the output (e.g. m_OnTrigger)
 * @param index			The index of the action to use
 * @param value			How many times to fire, or 

 * @return				True on success, false otherwise
 */
native bool SetOutputActionTimesToFire(int entity, const char[] output, int index, int value);

/**
 * Inserts a new action to an entity's output at a given index
 *
 * @param entity		Entity to use
 * @param output		The name of the output (e.g. m_OnTrigger)
 * @param target		Name of the entity(s) to cause the action in
 * @param targetintput	Name of the action to fire
 * @param parameter		The parameter to send, 0 if none
 * @param delay			Number of seconds to wait before firing the action
 * @param timestofire	Amount of times left for an action to fire, action is deleted when this reaches 0
 * @param index			Index that action should be inserted at, actions after this index are increased

 * @return				True on success, false otherwise
 */
native bool InsertOutputAction(int entity,
								const char[] output,
								const char[] target,
								const char[] targetinput,
								const char[] parameter,
								float delay,
								int timestofire,
								int index = 0);

/**
 * Removes an action from an entity's output at a given index
 *
 * @param entity		Entity to use
 * @param output		The name of the output (e.g. m_OnTrigger)
 * @param index			The index of the action to use

 * @return				True on success, false otherwise
 */
native bool RemoveOutputAction(int entity, const char[] output, int index);
								
/**
 * Do not edit below this line!
 */
public Extension __ext_outputinfo =
{
	name = "OutputInfo",
	file = "outputinfo.ext",
#if defined AUTOLOAD_EXTENSIONS
	autoload = 1,
#else
	autoload = 0,
#endif
#if defined REQUIRE_EXTENSIONS
	required = 1,
#else
	required = 0,
#endif
};

#if !defined REQUIRE_EXTENSIONS
public __ext_outputinfo_SetNTVOptional()
{
	MarkNativeAsOptional("GetOutputActionCount");
	MarkNativeAsOptional("GetOutputActionTarget");
	MarkNativeAsOptional("SetOutputActionTarget");
	MarkNativeAsOptional("GetOutputActionTargetInput");
	MarkNativeAsOptional("SetOutputActionTargetInput");
	MarkNativeAsOptional("GetOutputActionParameter");
	MarkNativeAsOptional("SetOutputActionParameter");
	MarkNativeAsOptional("GetOutputActionDelay");
	MarkNativeAsOptional("SetOutputActionDelay");
	MarkNativeAsOptional("GetOutputActionTimesToFire");
	MarkNativeAsOptional("SetOutputActionTimesToFire");
	MarkNativeAsOptional("InsertOutputAction");
	MarkNativeAsOptional("RemoveOutputAction");
}
#endif
