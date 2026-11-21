/** 
 * This file is part of TheQuestForTruth project for the Arduboy platform by Guillaume Elias
 * You can reuse it in accordance with the GNU LESSER GENERAL PUBLIC LICENSE (see LICENSE) 
 **/

#ifndef _SINGLETON_H_
#define _SINGLETON_H_

/** This should be called 'semi-singleton' as it is more just a "last-created-instance-holder" than a singleton.*/
template<typename T>
class Singleton {
private:
    static T* Instance;

public:
    Singleton() 
    { 
        Instance = static_cast<T*>(this); 
    }
    static T* instance();
};

template<typename T>
T* Singleton<T>::instance()
{
    return Instance;
}

template<typename T>
T* Singleton<T>::Instance = nullptr;

#endif
