/*BHEADER**********************************************************************
 * (c) 2000   The Regents of the University of California
 *
 * See the file COPYRIGHT_and_DISCLAIMER for a complete copyright
 * notice, contact person, and disclaimer.
 *
 * $Revision$
 *********************************************************************EHEADER*/
/******************************************************************************
 *
 *****************************************************************************/

#include "headers.h"

/*--------------------------------------------------------------------------
 *--------------------------------------------------------------------------*/

int
hypre_BoxMapEntrySetInfo( hypre_BoxMapEntry  *entry,
                          void               *info )
{
   int ierr = 0;

   hypre_BoxMapEntryInfo(entry) = info;

   return ierr;
}

/*--------------------------------------------------------------------------
 *--------------------------------------------------------------------------*/

int
hypre_BoxMapEntryGetInfo( hypre_BoxMapEntry  *entry,
                          void              **info_ptr )
{
   int ierr = 0;

   *info_ptr = hypre_BoxMapEntryInfo(entry);

   return ierr;
}

/*--------------------------------------------------------------------------
 *--------------------------------------------------------------------------*/

int
hypre_BoxMapEntryGetExtents( hypre_BoxMapEntry  *entry,
                             hypre_Index         imin,
                             hypre_Index         imax )
{
   int ierr = 0;
   hypre_IndexRef  entry_imin = hypre_BoxMapEntryIMin(entry);
   hypre_IndexRef  entry_imax = hypre_BoxMapEntryIMax(entry);
   int             d;

   for (d = 0; d < 3; d++)
   {
      hypre_IndexD(imin, d) = hypre_IndexD(entry_imin, d);
      hypre_IndexD(imax, d) = hypre_IndexD(entry_imax, d);
   }

   return ierr;
}

/*--------------------------------------------------------------------------
 *--------------------------------------------------------------------------*/

int
hypre_BoxMapCreate( int            max_nentries,
                    hypre_Index    global_imin,
                    hypre_Index    global_imax,
                    hypre_BoxMap **map_ptr )
{
   int ierr = 0;

   hypre_BoxMap   *map;
   hypre_IndexRef  global_imin_ref;
   hypre_IndexRef  global_imax_ref;
   int             d;
                          
   map = hypre_CTAlloc(hypre_BoxMap, 1);
   hypre_BoxMapMaxNEntries(map) = max_nentries;
   global_imin_ref = hypre_BoxMapGlobalIMin(map);
   global_imax_ref = hypre_BoxMapGlobalIMax(map);
   for (d = 0; d < 3; d++)
   {
      hypre_IndexD(global_imin_ref, d) = hypre_IndexD(global_imin, d);
      hypre_IndexD(global_imax_ref, d) = hypre_IndexD(global_imax, d);
      hypre_BoxMapIndexesD(map, d)     = NULL;
   }
   hypre_BoxMapNEntries(map) = 0;
   hypre_BoxMapEntries(map)  = hypre_CTAlloc(hypre_BoxMapEntry, max_nentries);
   hypre_BoxMapTable(map)    = NULL;
      
   *map_ptr = map;
      
   return ierr;
}

/*--------------------------------------------------------------------------
 *--------------------------------------------------------------------------*/

int
hypre_BoxMapIncSize( hypre_BoxMap *map,
                     int           inc_nentries )
{
   int ierr = 0;

   int                 max_nentries = hypre_BoxMapMaxNEntries(map);
   hypre_BoxMapEntry  *entries      = hypre_BoxMapEntries(map);

   max_nentries += inc_nentries;
   entries = hypre_TReAlloc(entries, hypre_BoxMapEntry, max_nentries);

   hypre_BoxMapMaxNEntries(map) = max_nentries;
   hypre_BoxMapEntries(map)     = entries;
      
   return ierr;
}

/*--------------------------------------------------------------------------
 *--------------------------------------------------------------------------*/

int
hypre_BoxMapAddEntry( hypre_BoxMap *map,
                      hypre_Index   imin,
                      hypre_Index   imax,
                      void         *info )
{
   int ierr = 0;

   int                 nentries = hypre_BoxMapNEntries(map);
   hypre_BoxMapEntry  *entries  = hypre_BoxMapEntries(map);
   hypre_BoxMapEntry  *entry;
   hypre_IndexRef      entry_imin;
   hypre_IndexRef      entry_imax;
   int                 d;

   entry = &entries[nentries];
   entry_imin = hypre_BoxMapEntryIMin(entry);
   entry_imax = hypre_BoxMapEntryIMax(entry);
   for (d = 0; d < 3; d++)
   {
      hypre_IndexD(entry_imin, d) = hypre_IndexD(imin, d);
      hypre_IndexD(entry_imax, d) = hypre_IndexD(imax, d);
   }
   hypre_BoxMapEntryInfo(entry) = info;
   hypre_BoxMapNEntries(map) = nentries + 1;

   return ierr;
}

/*--------------------------------------------------------------------------
 *--------------------------------------------------------------------------*/

int
hypre_BoxMapAssemble( hypre_BoxMap *map )
{
   int ierr = 0;

   int                         nentries = hypre_BoxMapNEntries(map);
   hypre_BoxMapEntry          *entries  = hypre_BoxMapEntries(map);
   hypre_BoxMapEntry         **table;
   int                        *indexes[3];
   int                         size[3];

   hypre_BoxMapEntry          *entry;
   hypre_IndexRef              entry_imin;
   hypre_IndexRef              entry_imax;

   int                         imin[3];
   int                         imax[3];
   int                         iminmax[2];
   int                         index_not_there;
   int                         b, d, i, j, k;
            
   /*------------------------------------------------------
    * Set up the indexes array
    *------------------------------------------------------*/

   for (d = 0; d < 3; d++)
   {
      indexes[d] = hypre_CTAlloc(int, 2*nentries);
      size[d] = 0;
   }

   for (b = 0; b < nentries; b++)
   {
      entry  = &entries[b];
      entry_imin = hypre_BoxMapEntryIMin(entry);
      entry_imax = hypre_BoxMapEntryIMax(entry);

      for (d = 0; d < 3; d++)
      {
         iminmax[0] = hypre_IndexD(entry_imin, d);
         iminmax[1] = hypre_IndexD(entry_imax, d) + 1;

         for (i = 0; i < 2; i++)
         {
            /* find the new index position in the indexes array */
            index_not_there = 1;
            for (j = 0; j < size[d]; j++)
            {
               if (iminmax[i] <= indexes[d][j])
               {
                  if (iminmax[i] == indexes[d][j])
                  {
                     index_not_there = 0;
                  }
                  break;
               }
            }

            /* if the index is already there, don't add it again */
            if (index_not_there)
            {
               for (k = size[d]; k > j; k--)
               {
                  indexes[d][k] = indexes[d][k-1];
               }
               indexes[d][j] = iminmax[i];
               size[d]++;
            }
         }
      }
   }

   for (d = 0; d < 3; d++)
   {
      size[d]--;
   }
      
   /*------------------------------------------------------
    * Set up the table
    *------------------------------------------------------*/
      
   table = hypre_CTAlloc(hypre_BoxMapEntry *, (size[0] * size[1] * size[2]));
      
   for (b = 0; b < nentries; b++)
   {
      entry = &entries[b];
      entry_imin = hypre_BoxMapEntryIMin(entry);
      entry_imax = hypre_BoxMapEntryIMax(entry);

      /* find the indexes corresponding to the current box */
      for (d = 0; d < 3; d++)
      {
         j = 0;

         while (hypre_IndexD(entry_imin, d) != indexes[d][j])
         {
            j++;
         }
         hypre_IndexD(imin, d) = j;

         while (hypre_IndexD(entry_imax, d) + 1 != indexes[d][j])
         {
            j++;
         }
         hypre_IndexD(imax, d) = j;
      }

      /* set up map table */
      for (k = imin[2]; k < imax[2]; k++)
      {
         for (j = imin[1]; j < imax[1]; j++)
         {
            for (i = imin[0]; i < imax[0]; i++)
            {
               table[((k) * size[1] + j) * size[0] + i] = entry;
            }
         }
      }
   }
      
   /*------------------------------------------------------
    * Set up the map
    *------------------------------------------------------*/

   hypre_TFree(hypre_BoxMapTable(map));
   hypre_BoxMapTable(map) = table;
   for (d = 0; d < 3; d++)
   {
      hypre_TFree(hypre_BoxMapIndexesD(map, d));
      hypre_BoxMapIndexesD(map, d) = indexes[d];
      hypre_BoxMapSizeD(map, d) = size[d];
      hypre_BoxMapLastIndexD(map, d) = 0;
   }

   return ierr;
}

/*--------------------------------------------------------------------------
 *--------------------------------------------------------------------------*/

int
hypre_BoxMapDestroy( hypre_BoxMap *map )
{
   int ierr = 0;
   int d;

   if (map)
   {
      hypre_TFree(hypre_BoxMapEntries(map));
      hypre_TFree(hypre_BoxMapTable(map));
      
      for (d = 0; d < 3; d++)
      {
         hypre_TFree(hypre_BoxMapIndexesD(map, d));
      }

      hypre_TFree(map);
   }

   return ierr;
}

/*--------------------------------------------------------------------------
 * This routine return a NULL 'entry_ptr' if an entry is not found
 *--------------------------------------------------------------------------*/

int
hypre_BoxMapFindEntry( hypre_BoxMap       *map,
                       hypre_Index         index,
                       hypre_BoxMapEntry **entry_ptr )
{
   int ierr = 0;

   int  index_d;
   int  map_index[3] = {0, 0, 0};
   int *map_indexes_d;
   int  map_index_d;
   int  map_size_d;
   int  d;
  
   for (d = 0; d < 3; d++)
   {
      map_indexes_d = hypre_BoxMapIndexesD(map, d);
      map_size_d    = hypre_BoxMapSizeD(map, d);

      /* Find location of index[d] in map */
      index_d = hypre_IndexD(index, d);

      /* Start looking in place indicated by last_index stored in map */
      map_index_d = hypre_BoxMapLastIndexD(map, d);

      /* Loop downward if target index is less than current location */
      while ( (map_index_d >= 0 ) &&
              (index_d < map_indexes_d[map_index_d]) )
      {
         map_index_d --;
      }

      /* Loop upward if target index is greater than current location */
      while ( (map_index_d <= (map_size_d-1)) &&
              (index_d >= map_indexes_d[map_index_d+1]) )
      {
         map_index_d ++;
      }

      if( ( map_index_d < 0 ) || ( map_index_d > (map_size_d-1) ) )
      {
         *entry_ptr = NULL;
         return ierr;
      }
      else
      {
         map_index[d] = map_index_d;
      }
   }

   /* If code reaches this point, then the entry was successfully found */
   *entry_ptr = hypre_BoxMapTableEntry(map,
                                       map_index[0],
                                       map_index[1],
                                       map_index[2]);

   /* Reset the last index in the map */
   for (d = 0; d < 3; d++)
   {
      hypre_BoxMapLastIndexD(map, d) = map_index[d];
   }

   return ierr;
}

/*--------------------------------------------------------------------------
 * This routine returns NULL for 'entries' if none are found
 *--------------------------------------------------------------------------*/

int
hypre_BoxMapIntersect( hypre_BoxMap        *map,
                       hypre_Index          ilower,
                       hypre_Index          iupper,
                       hypre_BoxMapEntry ***entries_ptr,
                       int                 *nentries_ptr )
{
   int ierr = 0;

   hypre_BoxMapEntry **entries;
   int                 nentries;

   int  index_d;
   int  map_ilower[3] = {0, 0, 0};
   int  map_iupper[3] = {0, 0, 0};
   int *map_indexes_d;
   int  map_index_d;
   int  map_size_d;
   int  d, i, j, k;
  
   for (d = 0; d < 3; d++)
   {
      map_indexes_d = hypre_BoxMapIndexesD(map, d);
      map_size_d    = hypre_BoxMapSizeD(map, d);

      /*------------------------------------------
       * Find location of ilower[d] in map
       *------------------------------------------*/

      index_d = hypre_IndexD(ilower, d);

      /* Start looking in place indicated by last_index stored in map */
      map_index_d = hypre_BoxMapLastIndexD(map, d);

      /* Loop downward if target index is less than current location */
      while ( (map_index_d >= 0 ) &&
              (index_d < map_indexes_d[map_index_d]) )
      {
         map_index_d --;
      }

      /* Loop upward if target index is greater than current location */
      while ( (map_index_d <= (map_size_d-1)) &&
              (index_d >= map_indexes_d[map_index_d+1]) )
      {
         map_index_d ++;
      }

      if( map_index_d > (map_size_d-1) )
      {
         *entries_ptr  = NULL;
         *nentries_ptr = 0;
         return ierr;
      }
      else
      {
         map_ilower[d] = hypre_max(map_index_d, 0);
      }

      /*------------------------------------------
       * Find location of iupper[d] in map
       *------------------------------------------*/

      index_d = hypre_IndexD(iupper, d);

      /* Loop upward if target index is greater than current location */
      while ( (map_index_d <= (map_size_d-1)) &&
              (index_d >= map_indexes_d[map_index_d+1]) )
      {
         map_index_d ++;
      }

      if( map_index_d < 0 )
      {
         *entries_ptr  = NULL;
         *nentries_ptr = 0;
         return ierr;
      }
      else
      {
         map_iupper[d] = hypre_min(map_index_d, (map_size_d-1)) + 1;
      }
   }

   /*---------------------------------------------
    * If code reaches this point, then set up the
    * entries array and eliminate duplicates.
    *---------------------------------------------*/

   nentries = ((map_iupper[0] - map_ilower[0]) *
               (map_iupper[1] - map_ilower[1]) *
               (map_iupper[2] - map_ilower[2]));
   entries  = hypre_CTAlloc(hypre_BoxMapEntry *, nentries);

   nentries = 0;
   for (k = map_ilower[2]; k < map_iupper[2]; k++)
   {
      for (j = map_ilower[1]; j < map_iupper[1]; j++)
      {
         for (i = map_ilower[0]; i < map_iupper[0]; i++)
         {
            /* the next 3 `if' statements eliminate duplicates */
            if (k > map_ilower[2])
            {
               if ( hypre_BoxMapTableEntry(map, i, j, k) ==
                    hypre_BoxMapTableEntry(map, i, j, (k-1)) )
               {
                  continue;
               }
            }
            if (j > map_ilower[1])
            {
               if ( hypre_BoxMapTableEntry(map, i, j, k) ==
                    hypre_BoxMapTableEntry(map, i, (j-1), k) )
               {
                  continue;
               }
            }
            if (i > map_ilower[0])
            {
               if ( hypre_BoxMapTableEntry(map, i, j, k) ==
                    hypre_BoxMapTableEntry(map, (i-1), j, k) )
               {
                  continue;
               }
            }

            entries[nentries] = hypre_BoxMapTableEntry(map, i, j, k);
            if (entries[nentries] != NULL)
            {
               nentries++;
            }
         }
      }
   }

   /* Reset the last index in the map */
   for (d = 0; d < 3; d++)
   {
      hypre_BoxMapLastIndexD(map, d) = map_ilower[d];
   }

   *entries_ptr  = entries;
   *nentries_ptr = nentries;

   return ierr;
}


