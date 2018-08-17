Custom Tags Plugin and API
==========================

The custom tags plugin provides a generic method for users to "tag" other users.

It is up to clients to infer a meaning to these tags. Example use-cases might be
blacklists, "likes", etc. . The blockchain logic is ignorant about the meaning
of the tags.

The plugin currently collects all tags from all users. Future versions may be
configurable to only accept tags from certain users, and/or to accept only
certain predefined tag names.

How to Tag
----------

A tag is created by publishing a [``custom_json_operation``](https://github.com/themuseblockchain/Muse-Source/blob/0.3.0/libraries/chain/include/muse/chain/protocol/base_operations.hpp#L365).

The ``custom_json_operation`` must fulfill these requirements in order to be considered as a tagging operation:

* It must contain exactly one authorizing account. Basic authority is sufficient.
* The contained JSON must be a valid JSON object.
* The contained JSON must contain the key *label* with a string value.
* The contained JSON must contain **either**
** the key *set* **or**
** the key *add* and/or the key *remove*.
* The keys *set*, *add* and *remove* must contain valid JSON arrays of existing account names, if present.

The authorizing account is taken as the person that owns (i. e. controls) the
tag. The *label* specifies the name of the tag, and the accounts specified in
*set*, *add* or *remove* are the accounts receiving the tag.

**Example**

1. Alice publishes ``{"label": "x", "set": ["bob"]}``
   Resulting tags: (alice,"x",bob)
2. Alice publishes ``{"label": "y", "add": ["charlie"]}``
   Resulting tags: (alice,"x",bob), (alice,"y",charlie)
3. Alice publishes ``{"label": "x", "add": ["charlie"], "remove": ["bob"]}
   Resulting tags: (alice,"x",charlie), (alice,"y",charlie)
4. Alice publishes ``{"label": "y", "set": ["dora"]}``
   Resulting tags: (alice,"x",charlie), (alice,"y",dora)

Tags API
--------

**Note:** this functionality is only present in API nodes that have enabled the
``custom_tags`` plugin!

**``is_tagged( user, tagged_by, tag )``**

Checks if account *tagged_by* owns a tag named *tag* on account *user*.
In other words, checks if *user* has received the *tag* from account *tagged_by*.

In the previous example, is_tagged("dora","alice","y") would return true.

**``list_tags_from( tagger, filter_tag, start_tag, start_taggee )``**

Returns a possibly filtered list of up to 100 known tags, ordered by (tag,taggee).

* *tagger* is the account name that has created the tags.
* if *filter_tag* is not empty, only entries with that tag name are returned
* *start_tag* and *start_taggee* can be provided if *tagger* has created more
  than 100 tags matching *filter_tag*, to retrieve next next 100 entries.

**``list_tags_on( taggee, filter_tag, start_tag, start_tagger )``**

Returns a possibly filtered list of up to 100 known tags, ordered by (tag,tagger).

* *taggee* is the account name that has received the tags.
* if *filter_tag* is not empty, only entries with that tag name are returned
* *start_tag* and *start_tagger* can be provided if *taggee* has received more
  than 100 tags matching *filter_tag*, to retrieve next next 100 entries.
