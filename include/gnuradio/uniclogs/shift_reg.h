#ifndef INCLUDED_UNICLOGS_SHIFT_REG_H
#define INCLUDED_UNICLOGS_SHIFT_REG_H

#include <gnuradio/uniclogs/api.h>
#include <deque>
#include <ostream>

namespace gr {
namespace uniclogs {


/*!
 * \brief Implements a bit shift register
 *
 */
class UNICLOGS_API shift_reg
{
public:
    shift_reg(size_t len);
    ~shift_reg();

    void reset();

    void set();

		void print();

    size_t len() const;

    size_t size() const;

    size_t count();

    shift_reg operator|(const shift_reg& rhs);

    shift_reg operator&(const shift_reg& rhs);

    shift_reg operator^(const shift_reg& rhs);

    shift_reg& operator>>=(bool bit);

    bool& operator[](size_t pos);

    bool operator[](size_t pos) const;

    shift_reg& operator<<=(bool bit);

    void push_front(bool bit);

    void push_back(bool bit);

    bool front();

    bool back();

    friend std::ostream& operator<<(std::ostream& os, const shift_reg& reg);

private:
    const size_t d_len;
    std::deque<bool> d_reg;
};


} // namespace uniclogs
} // namespace gr

#endif /* INCLUDED_UNICLOGS_SHIFT_REG_H */
