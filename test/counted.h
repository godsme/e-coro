//
// Created by Darwin Yuan on 2020/9/13.
//

#ifndef E_CORO_COUNTED_H
#define E_CORO_COUNTED_H

#include <iostream>

struct counted {
   static int default_construction_count;
   static int copy_construction_count;
   static int move_construction_count;
   static int destruction_count;

   int id;

   static auto reset_counts() {
      default_construction_count = 0;
      copy_construction_count = 0;
      move_construction_count = 0;
      destruction_count = 0;
   }

   static auto construction_count() {
      return default_construction_count + copy_construction_count + move_construction_count;
   }

   static auto active_count() {
      return construction_count() - destruction_count;
   }

   counted() : id(default_construction_count++) {
      //std::cout << "default cons: " << construction_count() << ":" << destruction_count << std::endl;
   }
   counted(counted const& other) : id(other.id) {
      ++copy_construction_count;
      //std::cout << "copy cons: " << construction_count() << ":" << destruction_count << std::endl;
   }
   counted(counted&& other) : id(other.id) {
      ++move_construction_count; other.id = -1;
      //std::cout << "move cons: " << construction_count() << ":" << destruction_count << std::endl;
   }
   ~counted() {
      ++destruction_count;
      //std::cout << " dtor: " << construction_count() << ":" << destruction_count << std::endl;
   }
};

#endif //E_CORO_COUNTED_H
