// copyright (C) 2005 nathaniel smith <njs@pobox.com>
// copyright (C) 2005 graydon hoare <graydon@pobox.com>
// all rights reserved.
// licensed to the public under the terms of the GNU GPL (>= 2)
// see the file COPYING for details

#include <set>

#include <boost/shared_ptr.hpp>

#include "diff_patch.hh"
#include "merge.hh"
#include "packet.hh"
#include "revision.hh"
#include "roster_merge.hh"
#include "safe_map.hh"
#include "transforms.hh"

using std::map;
using std::make_pair;
using boost::shared_ptr;

static void
get_file_details(roster_t const & ros, node_id nid,
                 file_id & fid,
                 file_path & pth)
{
  I(ros.has_node(nid));
  file_t f = downcast_to_file_t(ros.get_node(nid));
  fid = f->content;
  split_path sp;
  ros.get_name(nid, sp);
  pth = file_path(sp);
}

void
resolve_merge_conflicts(revision_id const & left_rid,
                        revision_id const & right_rid,
                        roster_t const & left_roster, 
                        roster_t const & right_roster,
                        marking_map const & left_marking_map, 
                        marking_map const & right_marking_map,
                        roster_merge_result & result,
                        content_merge_adaptor & adaptor,
                        app_state & app)
{
  // FIXME_ROSTERS: we only have code (below) to invoke the
  // line-merger on content conflicts. Other classes of conflict will
  // cause an invariant to trip below.  Probably just a bunch of lua
  // hooks for remaining conflict types will be ok.
  
  if (!result.is_clean())
    {
      L(F("unclean mark-merge: %d name conflicts, %d content conflicts, %d attr conflicts, "
          "%d orphaned node conflicts, %d rename target conflicts, %d directory loop conflicts\n") 
        % result.node_name_conflicts.size()
        % result.file_content_conflicts.size()
        % result.node_attr_conflicts.size()
        % result.orphaned_node_conflicts.size()
        % result.rename_target_conflicts.size()
        % result.directory_loop_conflicts.size());

      for (size_t i = 0; i < result.node_name_conflicts.size(); ++i)
        L(F("name conflict on node %d: [parent %d, self %s] vs. [parent %d, self %s]\n") 
          % result.node_name_conflicts[i].nid 
          % result.node_name_conflicts[i].left.first 
          % result.node_name_conflicts[i].left.second
          % result.node_name_conflicts[i].right.first 
          % result.node_name_conflicts[i].right.second);

      for (size_t i = 0; i < result.file_content_conflicts.size(); ++i)
        L(F("content conflict on node %d: [%s] vs. [%s]\n") 
          % result.file_content_conflicts[i].nid
          % result.file_content_conflicts[i].left
          % result.file_content_conflicts[i].right);

      for (size_t i = 0; i < result.node_attr_conflicts.size(); ++i)
        L(F("attribute conflict on node %d, key %s: [%d, %s] vs. [%d, %s]\n") 
          % result.node_attr_conflicts[i].nid
          % result.node_attr_conflicts[i].key
          % result.node_attr_conflicts[i].left.first
          % result.node_attr_conflicts[i].left.second
          % result.node_attr_conflicts[i].right.first
          % result.node_attr_conflicts[i].right.second);

      for (size_t i = 0; i < result.orphaned_node_conflicts.size(); ++i)
        L(F("orphaned node conflict on node %d, dead parent %d, name %s")
          % result.orphaned_node_conflicts[i].nid
          % result.orphaned_node_conflicts[i].parent_name.first
          % result.orphaned_node_conflicts[i].parent_name.second);

      for (size_t i = 0; i < result.rename_target_conflicts.size(); ++i)
        L(F("rename target conflict: nodes %d, %d, both want parent %d, name %s")
          % result.rename_target_conflicts[i].nid1
          % result.rename_target_conflicts[i].nid2
          % result.rename_target_conflicts[i].parent_name.first
          % result.rename_target_conflicts[i].parent_name.second);

      for (size_t i = 0; i < result.directory_loop_conflicts.size(); ++i)
        L(F("directory loop conflict: node %d, wanted parent %d, name %s")
          % result.directory_loop_conflicts[i].nid
          % result.directory_loop_conflicts[i].parent_name.first
          % result.directory_loop_conflicts[i].parent_name.second);
        
      // Attempt to auto-resolve any content conflicts using the line-merger.
      // To do this requires finding a merge ancestor.
      if (!result.file_content_conflicts.empty())
        {

          L(F("examining content conflicts\n"));
          std::vector<file_content_conflict> residual_conflicts;

          for (size_t i = 0; i < result.file_content_conflicts.size(); ++i)
            {
              file_content_conflict const & conflict = result.file_content_conflicts[i];

              boost::shared_ptr<roster_t> roster_for_file_lca;
              adaptor.get_ancestral_roster(conflict.nid, roster_for_file_lca);

              // Now we should certainly have a roster, which has the node.
              I(roster_for_file_lca);
              I(roster_for_file_lca->has_node(conflict.nid));

              file_id anc_id, left_id, right_id;
              file_path anc_path, left_path, right_path;
              get_file_details (*roster_for_file_lca, conflict.nid, anc_id, anc_path);
              get_file_details (left_roster, conflict.nid, left_id, left_path);
              get_file_details (right_roster, conflict.nid, right_id, right_path);
              
              file_id merged_id;
              
              content_merger cm(app, *roster_for_file_lca, 
                                left_roster, right_roster, 
                                adaptor);

              if (cm.try_to_merge_files(anc_path, left_path, right_path, right_path,
                                        anc_id, left_id, right_id, merged_id))
                {
                  L(F("resolved content conflict %d / %d\n") 
                    % (i+1) % result.file_content_conflicts.size());
                  file_t f = downcast_to_file_t(result.roster.get_node(conflict.nid));
                  f->content = merged_id;
                }
              else
                residual_conflicts.push_back(conflict);
            }
          result.file_content_conflicts = residual_conflicts;     
        }
    }

  E(result.is_clean(),
    F("merge failed due to unresolved conflicts\n"));
}

void
interactive_merge_and_store(revision_id const & left_rid,
                            revision_id const & right_rid,
                            revision_id & merged_rid,
                            app_state & app)
{
  roster_t left_roster, right_roster;
  marking_map left_marking_map, right_marking_map;
  std::set<revision_id> left_uncommon_ancestors, right_uncommon_ancestors;

  app.db.get_roster(left_rid, left_roster, left_marking_map);
  app.db.get_roster(right_rid, right_roster, right_marking_map);
  app.db.get_uncommon_ancestors(left_rid, right_rid,
                                left_uncommon_ancestors, right_uncommon_ancestors);

  roster_merge_result result;

//   {
//     data tmp;
//     write_roster_and_marking(left_roster, left_marking_map, tmp);
//     P(F("merge left roster: [[[\n%s\n]]]\n") % tmp);
//     write_roster_and_marking(right_roster, right_marking_map, tmp);
//     P(F("merge right roster: [[[\n%s\n]]]\n") % tmp);
//   }

  roster_merge(left_roster, left_marking_map, left_uncommon_ancestors,
               right_roster, right_marking_map, right_uncommon_ancestors,
               result);

  roster_t & merged_roster = result.roster;

  content_merge_database_adaptor dba(app, left_rid, right_rid, left_marking_map);
  resolve_merge_conflicts (left_rid, right_rid,
                           left_roster, right_roster,
                           left_marking_map, right_marking_map,
                           result, dba, app);

  // write new files into the db

  I(result.is_clean());
  merged_roster.check_sane();

  revision_set merged_rev;
  
  calculate_ident(merged_roster, merged_rev.new_manifest);
  
  boost::shared_ptr<cset> left_to_merged(new cset);
  make_cset(left_roster, merged_roster, *left_to_merged);
  safe_insert(merged_rev.edges, std::make_pair(left_rid, left_to_merged));
  
  boost::shared_ptr<cset> right_to_merged(new cset);
  make_cset(right_roster, merged_roster, *right_to_merged);
  safe_insert(merged_rev.edges, std::make_pair(right_rid, right_to_merged));
  
  revision_data merged_data;
  write_revision_set(merged_rev, merged_data);
  calculate_ident(merged_data, merged_rid);
  {
    transaction_guard guard(app.db);
  
    app.db.put_revision(merged_rid, merged_rev);
    packet_db_writer dbw(app);
    if (app.date_set)
      cert_revision_date_time(merged_rid, app.date, app, dbw);
    else
      cert_revision_date_now(merged_rid, app, dbw);
    if (app.author().length() > 0)
      cert_revision_author(merged_rid, app.author(), app, dbw);
    else
      cert_revision_author_default(merged_rid, app, dbw);

    guard.commit();
  }
}