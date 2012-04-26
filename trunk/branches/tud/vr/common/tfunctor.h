/**
   project: measure
   filename: tfunctor.h
   author: A.Bacioccola <a.bacioccola@iet.unipi.it>
   year: 2007
   affiliation:
      Dipartimento di Ingegneria dell'Informazione
      University of Pisa, Italy
   description:
      class to manage function pointer
 */

#ifndef ns_tfunctor_h
#define ns_tfunctor_h

//! abstract base class
class TFunctor {
        public:
        //! call using operator
        /** two possible functions to call member function. virtual cause 
         * derived classes will use a pointer to an object and a pointer to a 
         * member function to make the function call
         */
        virtual void operator()(void) = 0;

        //! call using function
        virtual void call(void) = 0;

        //! Virtual denstructor
        virtual ~TFunctor() { /* Do Nothing*/ }
};


//! derived template class
template <typename T>
class TSpecificFunctor : public TFunctor {
        public:
                //! constructor - takes pointer to an object and pointer to a 
                //! member and stores
                /** @param [in] _pt2Object Pointer to the class
                 *  @param [in] _fpt Pointer to a class function member
                 */
                TSpecificFunctor(T* _pt2Object, 
                                 void(T::*_fpt)(void));

                //! Denstructor
                virtual ~TSpecificFunctor() { /* Do Nothing */ }

                //! override operator "()"
                /** execute member function
                 */
                virtual void operator()(void);

                //! override function "Call"
                /** execute member function
                 */
                virtual void call(void);

        private:
                //! pointer to member function
                void (T::*fpt)(void);

                //! pointer to object
                T* pt2Object;
};

template <typename T>
TSpecificFunctor<T>::TSpecificFunctor( T* _pt2Object,
                                       void(T::*_fpt)(void) )
{
        pt2Object = _pt2Object;
        fpt=_fpt;
}

template <typename T>
void
TSpecificFunctor<T>::operator()(void)
{
        (*pt2Object.*fpt)();
}

template <typename T>
void
TSpecificFunctor<T>::call(void)
{ 
        (*pt2Object.*fpt)();
}

#endif