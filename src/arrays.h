/* Example:
typedef struct i32array
{
    i32 *data;
    sze count;
    sze capacity;
} i32array;

i32array x = {0};
*push(&x, &arena) = 3;

*/

#if COMPILER_MSVC
#define push(a, perm)  (((a)->count >= (a)->capacity) ? \
grow(a, sizeof(*(a)->data), alignof(typeid(*(a)->data)), perm), \
(a)->data + (a)->count++ : \
(a)->data + (a)->count++)
#else
#define push(a, perm)  (((a)->count >= (a)->capacity) ? \
grow(a, sizeof(*(a)->data), alignof(*(a)->data), perm), \
(a)->data + (a)->count++ : \
(a)->data + (a)->count++)
#endif

func void grow(void *arr, sze elemSize, sze alignSize, Arena *perm)
{
    struct {
        byte *data;
        sze count;
        sze capacity;
    } baseArr;
    memcpy(&baseArr, arr, sizeof(baseArr));

    if (baseArr.capacity == 0)
    {
        baseArr.capacity = 1;
        baseArr.data = alloc(perm, 2 * elemSize, alignSize, baseArr.capacity, 0);
    }
    else if (perm->begin == (baseArr.data + baseArr.capacity*elemSize))
    {
        alloc(perm, elemSize, 1, baseArr.capacity, 0);
    }
    else
    {
        void *newData = alloc(perm, 2*elemSize, alignSize, baseArr.capacity, 0);
        memcpy(newData, baseArr.data, baseArr.count * elemSize);
        baseArr.data = newData;
    }

    baseArr.capacity *= 2;

    memcpy(arr, &baseArr, sizeof(baseArr))
}
