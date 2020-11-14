/** Return index of element in a[nElements]; < 0 if not found. */
int
search_for_element(int a[], int nElements, int element)
{
  for(int index = 0; index < nElements; index++)
  {
    if (a[index] == element) return index;
  }
  return -1;
}
