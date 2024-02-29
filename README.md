With the original code, some new attributes had been added. Namely,
optional<uint64_t> parent; -> optional type - parent. Because most nodes will have a parent to refer to. 
optional type used as root doesn’t have parent, in which case it is not given a value.
For Inner nodes -> array size for the children - used to hold the children changed from kCapacity to kCapacity+1, as one child between each key, and smaller than first key, greater values than last key too present in the children.

lower bound function defined: which does binary search to find the value, the lowest instance/gets the index of such a key whose value is not less than the one being searched.

**Inner-Node:** Where all the keys, but no particular records/values present. Here, we use these nodes to search for a particular value efficiently.
    Insert: 
    we are passed a reference to the new node to be inserted, and a corresponding key to be inserted.  We initially copy each key, child for that node, and decide its position by using the comparator function. If a position was found, insert at that particular position. If a particular position wasn't found during the comparisons, i.e. we don’t have a particualr lower bound, we will just insert it at the end. 
    Lastly, copy everything back.

    Split: The split function splits a node into two, distributing keys and associated values between them. Calculates half/middle, and separates keys accordingly. Separator key is also calculated, when inner node is created Finally, returns the separator key for parent node usage. 

**Leaf-Node**: Different from Inner-Node such that it contains values too, here is where the actual ones present, and we search in the nodes, according to the directions obtained from the Inner Nodes.

    Insert: 
    Function is mostly similar to Inner-Node's function. We create copies, insert accordingly, and then copy back. Here only that if we find any duplicate key, then the value has to be over-written instead of being pushed. 

    Erase: 
    Erasing from a particular position just includes doing -> curr value = next element's value. Last element is removed.

    Split: 
    similar method, calculate middle, keys being divided, finally separator returned.

Moreover, for ease of searching, we also defined a removed data structure (with unordered_map) to keep track of key being erased. Now if any key erased, we just push in that. During searching/look-up too, we take care that if such a key was recently deleted, we dont waste the effort of searching.

**Look-up:**
As said before, if no tree or key in removed structure, just end searching. 
If root==leaf, means only one node, do BS to search. 
Else we do BS to get to the particular leaf. For that, lower_bound function defined before is employed.
Once we get to that key, search till we dont traverse everything. return based upon whether value found or not. 

Erase: It checks if root node present and returns if not. The function iteratively traverses the tree,. If the key is found - in a leaf node, it is removed and marked as removed - by pushing to removed map. 

Insert: 
based up on page being used, it is fixed firstly, then unfixed. 
If curr node is indeed a leaf:
    Check for space. If space is present, just insert it. Then unreleased.
    Else we create a new leaf node, and based up on whether it has parent or not, we check.
    If it doesn't have parent - which means it’s the root, then in such a case, we need to create and use the parent inner node, and handle both the old, and newly created leaf node. Moreover, a separator key, is also enabled, which will decide the structure of the B+ Tree.
Else: We first check whether capacity has been reached or not. If not, we can just use the lower bound function to determine the place and insert. If capacity reached, we would have to handle the 2 nodes. We handle the making of the new node, then get the separator, and get ready to update the children. They will be assigned the newly created node as their parent, due to the split. Here, thereafter quite a similar process to the above if part (If curr node is indeed a leaf) occurs, we check if parent existed or not, and take similar steps as defined before to handle it.
