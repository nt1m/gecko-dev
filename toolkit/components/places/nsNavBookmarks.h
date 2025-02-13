/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef nsNavBookmarks_h_
#define nsNavBookmarks_h_

#include "nsINavBookmarksService.h"
#include "nsITransaction.h"
#include "nsNavHistory.h"
#include "nsToolkitCompsCID.h"
#include "nsCategoryCache.h"
#include "nsTHashtable.h"
#include "nsWeakReference.h"
#include "mozilla/Attributes.h"
#include "prtime.h"

class nsNavBookmarks;

namespace mozilla {
namespace places {

  enum BookmarkStatementId {
    DB_FIND_REDIRECTED_BOOKMARK = 0
  , DB_GET_BOOKMARKS_FOR_URI
  };

  struct BookmarkData {
    int64_t id = -1;
    nsCString url;
    nsCString title;
    int32_t position = -1;
    int64_t placeId = -1;
    int64_t parentId = -1;
    int64_t grandParentId = -1;
    int32_t type = 0;
    int32_t syncStatus = nsINavBookmarksService::SYNC_STATUS_UNKNOWN;
    nsCString serviceCID;
    PRTime dateAdded = 0;
    PRTime lastModified = 0;
    nsCString guid;
    nsCString parentGuid;
  };

  struct ItemVisitData {
    BookmarkData bookmark;
    int64_t visitId;
    uint32_t transitionType;
    PRTime time;
  };

  struct ItemChangeData {
    BookmarkData bookmark;
    bool isAnnotation = false;
    bool updateLastModified = false;
    uint16_t source = nsINavBookmarksService::SOURCE_DEFAULT;
    nsCString property;
    nsCString newValue;
    nsCString oldValue;
  };

  struct TombstoneData {
    nsCString guid;
    PRTime dateRemoved;
  };

  typedef void (nsNavBookmarks::*ItemVisitMethod)(const ItemVisitData&);
  typedef void (nsNavBookmarks::*ItemChangeMethod)(const ItemChangeData&);

  enum BookmarkDate {
    LAST_MODIFIED
  };

} // namespace places
} // namespace mozilla

class nsNavBookmarks final : public nsINavBookmarksService
                           , public nsINavHistoryObserver
                           , public nsIObserver
                           , public nsSupportsWeakReference
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSINAVBOOKMARKSSERVICE
  NS_DECL_NSINAVHISTORYOBSERVER
  NS_DECL_NSIOBSERVER

  nsNavBookmarks();

  /**
   * Obtains the service's object.
   */
  static already_AddRefed<nsNavBookmarks> GetSingleton();

  /**
   * Initializes the service's object.  This should only be called once.
   */
  nsresult Init();

  static nsNavBookmarks* GetBookmarksService() {
    if (!gBookmarksService) {
      nsCOMPtr<nsINavBookmarksService> serv =
        do_GetService(NS_NAVBOOKMARKSSERVICE_CONTRACTID);
      NS_ENSURE_TRUE(serv, nullptr);
      NS_ASSERTION(gBookmarksService,
                   "Should have static instance pointer now");
    }
    return gBookmarksService;
  }

  typedef mozilla::places::BookmarkData BookmarkData;
  typedef mozilla::places::ItemVisitData ItemVisitData;
  typedef mozilla::places::ItemChangeData ItemChangeData;
  typedef mozilla::places::BookmarkStatementId BookmarkStatementId;

  nsresult OnVisit(nsIURI* aURI, int64_t aVisitId, PRTime aTime,
                   int64_t aSessionId, int64_t aReferringId,
                   uint32_t aTransitionType, const nsACString& aGUID,
                   bool aHidden, uint32_t aVisitCount,
                   uint32_t aTyped, const nsAString& aLastKnownTitle);

  nsresult ResultNodeForContainer(int64_t aID,
                                  nsNavHistoryQueryOptions* aOptions,
                                  nsNavHistoryResultNode** aNode);

  // Find all the children of a folder, using the given query and options.
  // For each child, a ResultNode is created and added to |children|.
  // The results are ordered by folder position.
  nsresult QueryFolderChildren(int64_t aFolderId,
                               nsNavHistoryQueryOptions* aOptions,
                               nsCOMArray<nsNavHistoryResultNode>* children);

  /**
   * Turns aRow into a node and appends it to aChildren if it is appropriate to
   * do so.
   *
   * @param aRow
   *        A Storage statement (in the case of synchronous execution) or row of
   *        a result set (in the case of asynchronous execution).
   * @param aOptions
   *        The options of the parent folder node. These are the options used
   *        to fill the parent node.
   * @param aChildren
   *        The children of the parent folder node.
   * @param aCurrentIndex
   *        The index of aRow within the results.  When called on the first row,
   *        this should be set to -1.
   */
  nsresult ProcessFolderNodeRow(mozIStorageValueArray* aRow,
                                nsNavHistoryQueryOptions* aOptions,
                                nsCOMArray<nsNavHistoryResultNode>* aChildren,
                                int32_t& aCurrentIndex);

  /**
   * The async version of QueryFolderChildren.
   *
   * @param aNode
   *        The folder node that will receive the children.
   * @param _pendingStmt
   *        The Storage pending statement that will be used to control async
   *        execution.
   */
  nsresult QueryFolderChildrenAsync(nsNavHistoryFolderResultNode* aNode,
                                    mozIStoragePendingStatement** _pendingStmt);

  /**
   * @return index of the new folder in aIndex, whether it was passed in or
   *         generated by autoincrement.
   *
   * @note If aFolder is -1, uses the autoincrement id for folder index.
   * @note aTitle will be truncated to TITLE_LENGTH_MAX
   */
  nsresult CreateContainerWithID(int64_t aId, int64_t aParent,
                                 const nsACString& aTitle,
                                 bool aIsBookmarkFolder,
                                 int32_t* aIndex,
                                 const nsACString& aGUID,
                                 uint16_t aSource,
                                 int64_t* aNewFolder);

  /**
   * Fetches information about the specified id from the database.
   *
   * @param aItemId
   *        Id of the item to fetch information for.
   * @param aBookmark
   *        BookmarkData to store the information.
   */
  nsresult FetchItemInfo(int64_t aItemId,
                         BookmarkData& _bookmark);

  /**
   * Notifies that a bookmark has been visited.
   *
   * @param aItemId
   *        The visited item id.
   * @param aData
   *        Details about the new visit.
   */
  void NotifyItemVisited(const ItemVisitData& aData);

  /**
   * Notifies that a bookmark has changed.
   *
   * @param aItemId
   *        The changed item id.
   * @param aData
   *        Details about the change.
   */
  void NotifyItemChanged(const ItemChangeData& aData);

  /**
   * Recursively builds an array of descendant folders inside a given folder.
   *
   * @param aFolderId
   *        The folder to fetch descendants from.
   * @param aDescendantFoldersArray
   *        Output array to put descendant folders id.
   */
  nsresult GetDescendantFolders(int64_t aFolderId,
                                nsTArray<int64_t>& aDescendantFoldersArray);

  static const int32_t kGetChildrenIndex_Guid;
  static const int32_t kGetChildrenIndex_Position;
  static const int32_t kGetChildrenIndex_Type;
  static const int32_t kGetChildrenIndex_PlaceID;
  static const int32_t kGetChildrenIndex_SyncStatus;

  static mozilla::Atomic<int64_t> sLastInsertedItemId;
  static void StoreLastInsertedId(const nsACString& aTable,
                                  const int64_t aLastInsertedId);

private:
  static nsNavBookmarks* gBookmarksService;

  ~nsNavBookmarks();

  /**
   * Checks whether or not aFolderId points to a live bookmark.
   *
   * @param aFolderId
   *        the item-id of the folder to check.
   * @return true if aFolderId points to live bookmarks, false otherwise.
   */
  bool IsLivemark(int64_t aFolderId);

  /**
   * Locates the root items in the bookmarks folder hierarchy assigning folder
   * ids to the root properties that are exposed through the service interface.
   */
  nsresult EnsureRoots();

  nsresult AdjustIndices(int64_t aFolder,
                         int32_t aStartIndex,
                         int32_t aEndIndex,
                         int32_t aDelta);

  nsresult AdjustSeparatorsSyncCounter(int64_t aFolderId,
                                       int32_t aStartIndex,
                                       int64_t aSyncChangeDelta);

  /**
   * Fetches properties of a folder.
   *
   * @param aFolderId
   *        Folder to count children for.
   * @param _folderCount
   *        Number of children in the folder.
   * @param _guid
   *        Unique id of the folder.
   * @param _parentId
   *        Id of the parent of the folder.
   *
   * @throws If folder does not exist.
   */
  nsresult FetchFolderInfo(int64_t aFolderId,
                           int32_t* _folderCount,
                           nsACString& _guid,
                           int64_t* _parentId);

  nsresult AddSyncChangesForBookmarksWithURL(const nsACString& aURL,
                                             int64_t aSyncChangeDelta);

  // Bumps the change counter for all bookmarks with |aURI|. This is used to
  // update tagged bookmarks when adding or changing a tag entry.
  nsresult AddSyncChangesForBookmarksWithURI(nsIURI* aURI,
                                             int64_t aSyncChangeDelta);

  // Bumps the change counter for all bookmarked URLs within |aFolderId|. This
  // is used to update tagged bookmarks when changing or removing a tag folder.
  nsresult AddSyncChangesForBookmarksInFolder(int64_t aFolderId,
                                              int64_t aSyncChangeDelta);

  // Inserts a tombstone for a removed synced item.
  nsresult InsertTombstone(const BookmarkData& aBookmark);

  // Inserts tombstones for removed synced items.
  nsresult InsertTombstones(const nsTArray<TombstoneData>& aTombstones);

  // Removes a stale synced bookmark tombstone.
  nsresult RemoveTombstone(const nsACString& aGUID);

  // Removes the Sync orphan annotation from a synced item, so that Sync doesn't
  // try to reparent the item once it sees the original parent. Only synced
  // bookmarks should have this anno, but we do this for all bookmarks because
  // the anno may be backed up and restored.
  nsresult PreventSyncReparenting(const BookmarkData& aBookmark);

  nsresult SetItemTitleInternal(BookmarkData& aBookmark,
                                const nsACString& aTitle,
                                int64_t aSyncChangeDelta);

  /**
   * This is an handle to the Places database.
   */
  RefPtr<mozilla::places::Database> mDB;

  nsMaybeWeakPtrArray<nsINavBookmarkObserver> mObservers;

  int64_t TagsRootId() {
    nsresult rv = EnsureRoots();
    NS_ENSURE_SUCCESS(rv, -1);
    return mTagsRoot;
  }

  // These are lazy loaded, so never access them directly, always use the
  // XPIDL getters or TagsRootId().
  int64_t mRoot;
  int64_t mMenuRoot;
  int64_t mTagsRoot;
  int64_t mUnfiledRoot;
  int64_t mToolbarRoot;
  int64_t mMobileRoot;

  inline bool IsRoot(int64_t aFolderId) {
    return aFolderId == mRoot || aFolderId == mMenuRoot ||
           aFolderId == mTagsRoot || aFolderId == mUnfiledRoot ||
           aFolderId == mToolbarRoot || aFolderId == mMobileRoot;
  }

  nsresult SetItemDateInternal(enum mozilla::places::BookmarkDate aDateType,
                               int64_t aSyncChangeDelta,
                               int64_t aItemId,
                               PRTime aValue);

  // Recursive method to build an array of folder's children
  nsresult GetDescendantChildren(int64_t aFolderId,
                                 const nsACString& aFolderGuid,
                                 int64_t aGrandParentId,
                                 nsTArray<BookmarkData>& aFolderChildrenArray);

  enum ItemType {
    BOOKMARK = TYPE_BOOKMARK,
    FOLDER = TYPE_FOLDER,
    SEPARATOR = TYPE_SEPARATOR,
  };

  /**
   * Helper to insert a bookmark in the database.
   *
   *  @param aItemId
   *         The itemId to insert, pass -1 to generate a new one.
   *  @param aPlaceId
   *         The placeId to which this bookmark refers to, pass nullptr for
   *         items that don't refer to an URI (eg. folders, separators, ...).
   *  @param aItemType
   *         The type of the new bookmark, see TYPE_* constants.
   *  @param aParentId
   *         The itemId of the parent folder.
   *  @param aIndex
   *         The position inside the parent folder.
   *  @param aTitle
   *         The title for the new bookmark.
   *         Pass a void string to set a NULL title.
   *  @param aDateAdded
   *         The date for the insertion.
   *  @param [optional] aLastModified
   *         The last modified date for the insertion.
   *         It defaults to aDateAdded.
   *
   *  @return The new item id that has been inserted.
   *
   *  @note This will also update last modified date of the parent folder.
   */
  nsresult InsertBookmarkInDB(int64_t aPlaceId,
                              enum ItemType aItemType,
                              int64_t aParentId,
                              int32_t aIndex,
                              const nsACString& aTitle,
                              PRTime aDateAdded,
                              PRTime aLastModified,
                              const nsACString& aParentGuid,
                              int64_t aGrandParentId,
                              nsIURI* aURI,
                              uint16_t aSource,
                              int64_t* _itemId,
                              nsACString& _guid);

  nsresult GetBookmarksForURI(nsIURI* aURI,
                              nsTArray<BookmarkData>& _bookmarks);

  class RemoveFolderTransaction final : public nsITransaction {
  public:
    RemoveFolderTransaction(int64_t aID, uint16_t aSource)
      : mID(aID)
      , mSource(aSource)
    {}

    NS_DECL_ISUPPORTS

    NS_IMETHOD DoTransaction() override {
      nsNavBookmarks* bookmarks = nsNavBookmarks::GetBookmarksService();
      NS_ENSURE_TRUE(bookmarks, NS_ERROR_OUT_OF_MEMORY);
      BookmarkData folder;
      nsresult rv = bookmarks->FetchItemInfo(mID, folder);
      // TODO (Bug 656935): store the BookmarkData struct instead.
      mParent = folder.parentId;
      mIndex = folder.position;

      rv = bookmarks->GetItemTitle(mID, mTitle);
      NS_ENSURE_SUCCESS(rv, rv);

      return bookmarks->RemoveItem(mID, mSource);
    }

    NS_IMETHOD UndoTransaction() override {
      nsNavBookmarks* bookmarks = nsNavBookmarks::GetBookmarksService();
      NS_ENSURE_TRUE(bookmarks, NS_ERROR_OUT_OF_MEMORY);
      int64_t newFolder;
      return bookmarks->CreateContainerWithID(mID, mParent, mTitle, true,
                                              &mIndex, EmptyCString(),
                                              mSource, &newFolder);
    }

    NS_IMETHOD RedoTransaction() override {
      return DoTransaction();
    }

    NS_IMETHOD GetIsTransient(bool* aResult) override {
      *aResult = false;
      return NS_OK;
    }

    NS_IMETHOD Merge(nsITransaction* aTransaction, bool* aResult) override {
      *aResult = false;
      return NS_OK;
    }

  private:
    ~RemoveFolderTransaction() {}

    int64_t mID;
    uint16_t mSource;
    MOZ_INIT_OUTSIDE_CTOR int64_t mParent;
    nsCString mTitle;
    MOZ_INIT_OUTSIDE_CTOR int32_t mIndex;
  };

  // Used to enable and disable the observer notifications.
  bool mCanNotify;

  // Tracks whether we are in batch mode.
  // Note: this is only tracking bookmarks batches, not history ones.
  bool mBatching;
};

#endif // nsNavBookmarks_h_
