// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __SINGLETON_H__
#define __SINGLETON_H__

static sdLock singletonLock;

template < typename T > class sdSingleton {
public:
	sdSingleton( void ) { ; }

private:
	~sdSingleton( void ) {
		DestroyInstance();
	}

public:
	static T& GetInstance( void ) {
		if ( !instance ) {

			singletonLock.Acquire();

			if ( !instance ) {
				instance = new T;
			}

			singletonLock.Release();
		}
		return *instance;
	}

	static void DestroyInstance( void ) {
		if ( instance ) {
			delete instance;
			instance = NULL;
		}
	}

private:
	static T* instance;
};

template < typename T > T* sdSingleton< T >::instance = NULL;

#endif /* !__SINGLETON_H__ */
