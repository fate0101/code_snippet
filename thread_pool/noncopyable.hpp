#ifndef noncopyable_hpp
#define noncopyable_hpp

class noncopyable {
   protected:
      noncopyable() {}
      ~noncopyable() {}
   private:  // emphasize the following members are private
      noncopyable( const noncopyable& );
      const noncopyable& operator=( const noncopyable& );
  };
}

#endif  // noncopyable_hpp