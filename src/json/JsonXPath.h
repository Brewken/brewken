/*======================================================================================================================
 * json/JsonXPath.h is part of Brewken, and is copyright the following authors 2022:
 *   • Matt Young <mfsy@yahoo.com>
 *
 * Brewken is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * Brewken is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 =====================================================================================================================*/
#ifndef JSON_JSONXPATH_H
#define JSON_JSONXPATH_H
#pragma once

#include <string>
#include <string_view>
#include <vector>

/**
 * \brief \c JsonXPath is, essentially, almost the same as a JSON Pointer (see
 *        https://datatracker.ietf.org/doc/html/rfc6901) with the exception that the leading '/' character is optional.
 *
 *        Essentially, this gives us something very akin to XML's XPath.
 *
 *        We have a couple of motivations for omitting the leading '/' character and one motivation for using a rather
 *        different name (XPath rather than Pointer).
 *
 *        JSON Pointers are defined in terms of a JSON document, but any non-leaf node in a JSON document tree can be
 *        treated as a JSON document (at least for the purposes of navigation) and, eg, Boost.JSON supports using a JSON
 *        Pointer as a relative reference from any node.  So, when we are dealing with relative paths in the document
 *        tree, the leading '/' on a JSON Pointer can sometimes be a bit confusing.
 *
 *        Just as importantly, where a relative JSON Pointer refers to a key directly inside the current object, we'd
 *        like it to have the same syntax as accessing that key directly.  Eg, suppose the node we are looking at in a
 *        JSON document includes the following:
 *           "name": "Super Hops",
 *           "origin": "Planet Krypton",
 *           "alpha_acid": {
 *             "unit": "%",
 *             "value": 4.5
 *           }
 *        We would like to be able to refer to "name", "origin", "alpha_acid", "alpha_acid/unit" and "alpha_acid/value".
 *        We don't want to have to distinguish between "/name" and "name" depending on whether we are accessing that
 *        property via key:value pair or JSON Pointer.  (Of course, we could just put a '/' at the front of everything,
 *        but it seems redundant, especially as, in reality, >90% of the references we make are to direct children of
 *        the current node.
 *
 *        Finally, we prefer XPath over Pointer because the former is unambiguous (and has a valid strong analogy with a
 *        file system path).
 */
class JsonXPath {
public:
   /**
    * \brief Constructor
    */
   JsonXPath(char const * const xPath);
   ~JsonXPath();

   /**
    * \brief This returns a \c string_view because that's what we're going to pass to Boost.JSON
    */
   std::string_view asJsonPtr() const;

   /**
    * \brief For a trivial path, return it without the leading slash (as a \c string_view because that's what we're
    *        going to pass to Boost.JSON).  Caller's responsibility to ensure this is indeed a trivial path.
    */
   std::string_view asKey() const;

   /**
    * \return All the elements of the path as a list (without the '/' separators)
    *
    *         (It's std::string rather than std::string_view in the vector because I couldn't get the splitting code to
    *         compile on all platforms with std::string_view.)
    */
   std::vector<std::string> getElements() const;

   /**
    * \brief This returns a C-style string as that's most universally usable for logging
    */
   char const * asXPath_c_str() const;

private:
   /**
    * \brief The (relative) JSON Pointer to which this JsonXPath corresponds (ie same as the JsonXPath but with a / at
    *        the start).
    *
    *        Normally we would use \c QString by default, but, since the value is going to be passed to Boost.JSON, it's
    *        more convenient to store it as \c std::string and convert to char const * for logging.
    *
    *        We don't make this const as we want to store \c JsonXPath objects inside (structs inside) a vector, and
    *        anything you put in a vector needs to be CopyConstructible and Assignable.
    */
   std::string valueAsJsonPointer;
};

/**
 * \brief Convenience function for logging
 */
template<class S>
S & operator<<(S & stream, JsonXPath const & jsonXPath) {
   stream << jsonXPath.asXPath_c_str();
   return stream;
}

#endif
