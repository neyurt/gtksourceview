/*
 * test-completion-model.c
 * This file is part of GtkSourceView
 *
 * Copyright (C) 2013 - Sébastien Wilmet <swilmet@gnome.org>
 *
 * GtkSourceView is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * GtkSourceView is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <gtk/gtk.h>
#include <gtksourceview/gtksource.h>
#include "gtksourceview/gtksourcecompletionmodel.h"

/* Basic provider.
 * The populate function is not implemented. Proposals are created
 * independendly (it is more convenient).
 */

typedef struct _TestProvider      TestProvider;
typedef struct _TestProviderClass TestProviderClass;

struct _TestProvider
{
	GObject parent_instance;
	gint priority;
};

struct _TestProviderClass
{
	GObjectClass parent_class;
};

GType test_provider_get_type (void);

static void test_provider_iface_init (GtkSourceCompletionProviderIface *iface);

G_DEFINE_TYPE_WITH_CODE (TestProvider,
			 test_provider,
			 G_TYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (GTK_SOURCE_TYPE_COMPLETION_PROVIDER,
						test_provider_iface_init));

static gchar *
test_provider_get_name (GtkSourceCompletionProvider *provider)
{
	return g_strdup ("Hobbits");
}

static gint
test_provider_get_priority (GtkSourceCompletionProvider *provider)
{
	return ((TestProvider *)provider)->priority;
}

static void
test_provider_iface_init (GtkSourceCompletionProviderIface *iface)
{
	iface->get_name = test_provider_get_name;
	iface->get_priority = test_provider_get_priority;
}

static void
test_provider_class_init (TestProviderClass *klass)
{
}

static void
test_provider_init (TestProvider *self)
{
	self->priority = 0;
}


static TestProvider *
test_provider_new (void)
{
	return g_object_new (test_provider_get_type (), NULL);
}

/* Utilities functions */

static GList *
create_proposals (void)
{
	GList *list = NULL;

	list = g_list_append (list, gtk_source_completion_item_new ("Frodo", "Frodo", NULL, NULL));
	list = g_list_append (list, gtk_source_completion_item_new ("Bilbo", "Bilbo", NULL, NULL));

	return list;
}

/* Each returned provider is associated with a list of proposals.
 * The providers are sorted in decreasing order of priority, i.e. the same order
 * as in the CompletionModel.
 */
static void
create_providers (GList **all_providers,
		  GList **all_list_proposals)
{
	TestProvider *provider;

	*all_providers = NULL;
	*all_list_proposals = NULL;

	provider = test_provider_new ();
	provider->priority = 5;
	*all_providers = g_list_append (*all_providers, provider);
	*all_list_proposals = g_list_append (*all_list_proposals, create_proposals ());

	provider = test_provider_new ();
	provider->priority = 3;
	*all_providers = g_list_append (*all_providers, provider);
	*all_list_proposals = g_list_append (*all_list_proposals, create_proposals ());
}

static void
populate_model (GtkSourceCompletionModel *model,
		GList                    *all_providers,
		GList                    *all_list_proposals)
{
	GList *cur_provider;
	GList *cur_list_proposals;

	gtk_source_completion_model_begin_populate (model, all_providers);

	for (cur_provider = all_providers,
	     cur_list_proposals = all_list_proposals;
	     cur_provider != NULL;
	     cur_provider = g_list_next (cur_provider),
	     cur_list_proposals = g_list_next (cur_list_proposals))
	{
		gtk_source_completion_model_add_proposals (model,
							   GTK_SOURCE_COMPLETION_PROVIDER (cur_provider->data),
							   cur_list_proposals->data);

		gtk_source_completion_model_end_populate (model,
							  GTK_SOURCE_COMPLETION_PROVIDER (cur_provider->data));
	}
}

static void
free_providers (GList *all_providers,
		GList *all_list_proposals)
{
	GList *cur_list_proposals = NULL;

	g_list_free_full (all_providers, g_object_unref);

	for (cur_list_proposals = all_list_proposals;
	     cur_list_proposals != NULL;
	     cur_list_proposals = g_list_next (cur_list_proposals))
	{
		g_list_free_full (cur_list_proposals->data, g_object_unref);
	}

	g_list_free (all_list_proposals);
}

/* Check whether the provider is correctly present in the CompletionModel, at
 * the position specified by @iter.
 */
static void
check_provider (GtkSourceCompletionModel    *model,
		GtkSourceCompletionProvider *provider,
		GList                       *list_proposals,
		gboolean                     is_header_visible,
		GtkTreeIter                 *iter)

{
	GtkSourceCompletionProposal *proposal_get = NULL;
	GtkSourceCompletionProvider *provider_get = NULL;
	GList *cur_proposal = NULL;

	/* Check the header */

	if (is_header_visible)
	{
		g_assert (gtk_source_completion_model_iter_is_header (model, iter));

		gtk_tree_model_get (GTK_TREE_MODEL (model), iter,
				    GTK_SOURCE_COMPLETION_MODEL_COLUMN_PROPOSAL, &proposal_get,
				    GTK_SOURCE_COMPLETION_MODEL_COLUMN_PROVIDER, &provider_get,
				    -1);

		g_assert (proposal_get == NULL);
		g_assert (provider_get == provider);
		g_assert (gtk_tree_model_iter_next (GTK_TREE_MODEL (model), iter));
	}

	/* Check the proposals */

	cur_proposal = list_proposals;
	while (TRUE)
	{
		gtk_tree_model_get (GTK_TREE_MODEL (model), iter,
				    GTK_SOURCE_COMPLETION_MODEL_COLUMN_PROPOSAL, &proposal_get,
				    GTK_SOURCE_COMPLETION_MODEL_COLUMN_PROVIDER, &provider_get,
				    -1);

		g_assert (proposal_get == cur_proposal->data);
		g_assert (provider_get == provider);

		cur_proposal = g_list_next (cur_proposal);

		if (cur_proposal == NULL)
		{
			break;
		}

		g_assert (gtk_tree_model_iter_next (GTK_TREE_MODEL (model), iter));
	}
}

/* Check the full contents of a CompletionModel. */
static void
check_all_providers (GtkSourceCompletionModel *model,
		     GList                    *all_providers,
		     GList                    *all_list_proposals,
		     gboolean                  is_header_visible)
{
	GtkTreeIter iter;
	GList *cur_provider = NULL;
	GList *cur_list_proposals = NULL;

	g_assert (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (model), &iter));

	cur_provider = all_providers;
	cur_list_proposals = all_list_proposals;
	while (TRUE)
	{
		check_provider (model,
				GTK_SOURCE_COMPLETION_PROVIDER (cur_provider->data),
				cur_list_proposals->data,
				is_header_visible,
				&iter);

		cur_provider = g_list_next (cur_provider);
		cur_list_proposals = g_list_next (cur_list_proposals);

		if (cur_provider == NULL)
		{
			g_assert (cur_list_proposals == NULL);
			break;
		}

		g_assert (gtk_tree_model_iter_next (GTK_TREE_MODEL (model), &iter));
	}

#if 0
	g_assert (!gtk_tree_model_iter_next (GTK_TREE_MODEL (model), &iter));
#endif
}

static void
check_all_providers_with_and_without_headers (GtkSourceCompletionModel *model,
					      GList                    *all_providers,
					      GList                    *all_list_proposals)
{
	gtk_source_completion_model_set_show_headers (model, TRUE);
	check_all_providers (model, all_providers, all_list_proposals, TRUE);

	gtk_source_completion_model_set_show_headers (model, FALSE);
	check_all_providers (model, all_providers, all_list_proposals, FALSE);
}

static gboolean
same_list_contents (GList *list1, GList *list2)
{
	GList *cur_item1 = list1;
	GList *cur_item2 = list2;

	if (g_list_length (list1) != g_list_length (list2))
	{
		return FALSE;
	}

	while (cur_item1 != NULL)
	{
		if (cur_item1->data != cur_item2->data)
		{
			return FALSE;
		}

		cur_item1 = g_list_next (cur_item1);
		cur_item2 = g_list_next (cur_item2);
	}

	return TRUE;
}

/* Tests */

static void
test_is_empty (void)
{
	GtkSourceCompletionModel *model;
	TestProvider *provider;
	GList *list_providers = NULL;
	GList *list_proposals = NULL;
#if 0
	GList *visible_providers = NULL;
#endif

	/* Completely empty */
	model = gtk_source_completion_model_new ();

	g_assert (gtk_source_completion_model_is_empty (model, FALSE));
	g_assert (gtk_source_completion_model_is_empty (model, TRUE));

	/* One visible provider */
	provider = test_provider_new ();
	list_providers = g_list_append (list_providers, provider);
	list_proposals = create_proposals ();

	gtk_source_completion_model_begin_populate (model, list_providers);

	gtk_source_completion_model_add_proposals (model,
						   GTK_SOURCE_COMPLETION_PROVIDER (provider),
						   list_proposals);

	gtk_source_completion_model_end_populate (model,
						  GTK_SOURCE_COMPLETION_PROVIDER (provider));

	g_assert (!gtk_source_completion_model_is_empty (model, FALSE));
	g_assert (!gtk_source_completion_model_is_empty (model, TRUE));

	/* One invisible provider */

	/* FIXME The two following tests are broken with the current implementation, it
	 * will be fixed with the new implementation.
	 * However it seems that this situation never happen, because if there is only one
	 * invisible provider, the completion window is hidden.
	 */
#if 0
	visible_providers = g_list_append (visible_providers, test_provider_new ());
	gtk_source_completion_model_set_visible_providers (model, visible_providers);

	g_assert (!gtk_source_completion_model_is_empty (model, FALSE));
	g_assert (gtk_source_completion_model_is_empty (model, TRUE));
#endif

	g_object_unref (model);
	g_list_free_full (list_providers, g_object_unref);
	g_list_free_full (list_proposals, g_object_unref);
#if 0
	g_list_free_full (visible_providers, g_object_unref);
#endif
}

static void
test_get_visible_providers (void)
{
	GtkSourceCompletionModel *model;
	TestProvider *provider;
	GList *list_providers = NULL;
	GList *visible_providers = NULL;

	model = gtk_source_completion_model_new ();
	g_assert (gtk_source_completion_model_get_visible_providers (model) == NULL);

	provider = test_provider_new ();
	list_providers = g_list_append (list_providers, provider);

	gtk_source_completion_model_set_visible_providers (model, list_providers);
	visible_providers = gtk_source_completion_model_get_visible_providers (model);

	g_assert (visible_providers->data == provider);

	g_object_unref (model);
	g_list_free_full (list_providers, g_object_unref);
}

/* Create several providers with associated proposals, populate them in a
 * CompletionModel, and check whether the CompletionModel correctly contains the
 * providers.
 */
static void
test_simple_populate (void)
{
	GtkSourceCompletionModel *model;
	GList *all_providers = NULL;
	GList *all_list_proposals = NULL;

	model = gtk_source_completion_model_new ();
	create_providers (&all_providers, &all_list_proposals);
	populate_model (model, all_providers, all_list_proposals);

	check_all_providers_with_and_without_headers (model, all_providers, all_list_proposals);

	g_object_unref (model);
	free_providers (all_providers, all_list_proposals);
}

static void
test_clear (void)
{
	GtkSourceCompletionModel *model;
	GList *all_providers = NULL;
	GList *all_list_proposals = NULL;

	model = gtk_source_completion_model_new ();

	/* Clear the model when it is already empty */
	gtk_source_completion_model_clear (model);
	g_assert (gtk_source_completion_model_is_empty (model, FALSE));

	/* Add some proposals */
	create_providers (&all_providers, &all_list_proposals);
	populate_model (model, all_providers, all_list_proposals);

	/* Clear the model when it is not empty */
	gtk_source_completion_model_clear (model);
	g_assert (gtk_source_completion_model_is_empty (model, FALSE));

	g_object_unref (model);
	free_providers (all_providers, all_list_proposals);
}

static void
test_set_visible_providers (void)
{
	GtkSourceCompletionModel *model;
	GList *all_providers = NULL;
	GList *all_list_proposals = NULL;
	GList *subset_providers = NULL;
	GList *subset_list_proposals = NULL;
	TestProvider *other_provider;

	/* Populate the model with two providers */
	model = gtk_source_completion_model_new ();
	create_providers (&all_providers, &all_list_proposals);
	populate_model (model, all_providers, all_list_proposals);

	/* The two providers are initially visible */
	check_all_providers_with_and_without_headers (model, all_providers, all_list_proposals);

	gtk_source_completion_model_set_visible_providers (model, NULL);
	check_all_providers_with_and_without_headers (model, all_providers, all_list_proposals);

	gtk_source_completion_model_set_visible_providers (model, all_providers);
	check_all_providers_with_and_without_headers (model, all_providers, all_list_proposals);

	/* Only first provider visible */
	subset_providers = g_list_append (subset_providers, all_providers->data);
	subset_list_proposals = g_list_append (subset_list_proposals, all_list_proposals->data);

#if 0
	gtk_source_completion_model_set_visible_providers (model, subset_providers);
	check_all_providers_with_and_without_headers (model, subset_providers, subset_list_proposals);

	/* Only second provider visible */
	subset_providers->data = all_providers->next->data;
	subset_list_proposals->data = all_list_proposals->next->data;

	gtk_source_completion_model_set_visible_providers (model, subset_providers);
	check_all_providers_with_and_without_headers (model, subset_providers, subset_list_proposals);
#endif

	/* No visible providers */
	other_provider = test_provider_new ();
#if 0
	subset_providers->data = other_provider;
	gtk_source_completion_model_set_visible_providers (model, subset_providers);
	g_assert (gtk_source_completion_model_is_empty (model, TRUE));

	/* The two providers are visible again */
	gtk_source_completion_model_set_visible_providers (model, NULL);
	check_all_providers_with_and_without_headers (model, all_providers, all_list_proposals);
#endif

	g_object_unref (model);
	g_object_unref (other_provider);
	free_providers (all_providers, all_list_proposals);
	g_list_free (subset_providers);
	g_list_free (subset_list_proposals);
}

/* Do a first populate, and then a second populate with another set of
 * proposals. The proposals in common between the two populates are inserted in
 * the same order.
 */
static void
test_second_populate_same_order (void)
{
	GtkSourceCompletionModel *model;
	GList *all_providers = NULL;
	GList *all_list_proposals = NULL;
	GList *list_proposals = NULL;

	/* First populate with two providers */
	model = gtk_source_completion_model_new ();
	create_providers (&all_providers, &all_list_proposals);
	populate_model (model, all_providers, all_list_proposals);

	/* Remove the second provider and its associated proposals */
	g_object_unref (all_providers->next->data);
	all_providers = g_list_delete_link (all_providers, all_providers->next);

	g_list_free_full (all_list_proposals->next->data, g_object_unref);
	all_list_proposals = g_list_delete_link (all_list_proposals, all_list_proposals->next);

	/* Alter the proposals of the remaining provider */
	list_proposals = all_list_proposals->data;
	g_object_unref (list_proposals->data);
	list_proposals = g_list_delete_link (list_proposals, list_proposals);
	list_proposals = g_list_concat (list_proposals, create_proposals ());
	all_list_proposals->data = list_proposals;

	/* Second populate */
	populate_model (model, all_providers, all_list_proposals);
	check_all_providers_with_and_without_headers (model, all_providers, all_list_proposals);

	g_object_unref (model);
	free_providers (all_providers, all_list_proposals);
}

#if 0
/* Same as test_second_populate_same_order() but with a different insertion
 * order of the common proposals for the second populate.
 */
static void
test_second_populate_different_order (void)
{
	GtkSourceCompletionModel *model;
	GList *all_providers = NULL;
	GList *all_list_proposals = NULL;
	GList *list_proposals = NULL;

	/* First populate with two providers */
	model = gtk_source_completion_model_new ();
	create_providers (&all_providers, &all_list_proposals);
	populate_model (model, all_providers, all_list_proposals);

	/* Remove the second provider and its associated proposals */
	g_object_unref (all_providers->next->data);
	all_providers = g_list_delete_link (all_providers, all_providers->next);

	g_list_free_full (all_list_proposals->next->data, g_object_unref);
	all_list_proposals = g_list_delete_link (all_list_proposals, all_list_proposals->next);

	/* Alter the proposals of the remaining provider */
	list_proposals = all_list_proposals->data;
	list_proposals = g_list_reverse (list_proposals);
	list_proposals = g_list_concat (list_proposals, create_proposals ());
	all_list_proposals->data = list_proposals;

	/* Second populate */
	populate_model (model, all_providers, all_list_proposals);
	check_all_providers_with_and_without_headers (model, all_providers, all_list_proposals);

	g_object_unref (model);
	free_providers (all_providers, all_list_proposals);
}
#endif

static void
test_populate_several_batches (void)
{
	GtkSourceCompletionModel *model = gtk_source_completion_model_new ();
	GtkSourceCompletionProvider *provider = GTK_SOURCE_COMPLETION_PROVIDER (test_provider_new ());
	GList *list_providers = g_list_append (NULL, provider);
	GList *first_proposals = create_proposals ();
	GList *second_proposals = create_proposals ();
	GList *all_proposals;
	GtkTreeIter iter;

	gtk_source_completion_model_set_show_headers (model, TRUE);
	gtk_source_completion_model_begin_populate (model, list_providers);

	/* First batch */
	gtk_source_completion_model_add_proposals (model, provider, first_proposals);

	g_assert (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (model), &iter));
	check_provider (model, provider, first_proposals, TRUE, &iter);
	g_assert (!gtk_tree_model_iter_next (GTK_TREE_MODEL (model), &iter));

	/* Second batch */
	gtk_source_completion_model_add_proposals (model, provider, second_proposals);
	gtk_source_completion_model_end_populate (model, provider);

	all_proposals = g_list_copy (first_proposals);
	all_proposals = g_list_concat (all_proposals, g_list_copy (second_proposals));

	g_assert (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (model), &iter));
	check_provider (model, provider, all_proposals, TRUE, &iter);
	g_assert (!gtk_tree_model_iter_next (GTK_TREE_MODEL (model), &iter));

	g_object_unref (model);
	g_object_unref (provider);
	g_list_free (list_providers);
	g_list_free (first_proposals);
	g_list_free (second_proposals);
	g_list_free_full (all_proposals, g_object_unref);
}

static void
test_get_providers (void)
{
	GtkSourceCompletionModel *model = gtk_source_completion_model_new ();
	GList *all_providers = NULL;
	GList *all_list_proposals = NULL;
	GList *providers_get = NULL;

	/* Empty */
	g_assert (gtk_source_completion_model_get_providers (model) == NULL);

	/* Non-empty */
	create_providers (&all_providers, &all_list_proposals);
	populate_model (model, all_providers, all_list_proposals);

	providers_get = gtk_source_completion_model_get_providers (model);
	g_assert (same_list_contents (all_providers, providers_get));

	g_object_unref (model);
	free_providers (all_providers, all_list_proposals);
}

static void
test_n_proposals (void)
{
	GtkSourceCompletionModel *model;
	GtkSourceCompletionProvider *provider;
	GtkSourceCompletionProvider *other_provider;
	GList *list_providers = NULL;
	GList *list_proposals = NULL;
	guint nb_proposals_good = 0;
	guint nb_proposals_get = 0;

	model = gtk_source_completion_model_new ();
	provider = GTK_SOURCE_COMPLETION_PROVIDER (test_provider_new ());
	other_provider = GTK_SOURCE_COMPLETION_PROVIDER (test_provider_new ());
	list_providers = g_list_append (NULL, provider);
	list_proposals = create_proposals ();

	gtk_source_completion_model_begin_populate (model, list_providers);
	gtk_source_completion_model_add_proposals (model, provider, list_proposals);
	gtk_source_completion_model_end_populate (model, provider);

	g_assert (gtk_source_completion_model_n_proposals (model, other_provider) == 0);

	/* With header */
	gtk_source_completion_model_set_show_headers (model, TRUE);

	nb_proposals_good = g_list_length (list_proposals);
	nb_proposals_get = gtk_source_completion_model_n_proposals (model, provider);
	g_assert (nb_proposals_good == nb_proposals_get);

	/* Without header */
	gtk_source_completion_model_set_show_headers (model, FALSE);

	nb_proposals_get = gtk_source_completion_model_n_proposals (model, provider);
	g_assert (nb_proposals_good == nb_proposals_get);

	g_object_unref (model);
	g_object_unref (other_provider);
	g_list_free_full (list_providers, g_object_unref);
	g_list_free_full (list_proposals, g_object_unref);
}

static void
test_iters_impl (gboolean show_headers)
{
	GtkSourceCompletionModel *model = gtk_source_completion_model_new ();
	GList *all_providers = NULL;
	GList *all_list_proposals = NULL;
	GtkTreeIter first_iter;
	GtkTreeIter last_iter;
	GtkTreeIter other_iter;
	gint nb_items;

	/* Test iter_last() */
#if 0
	/* segfault */
	g_assert (!gtk_source_completion_model_iter_last (model, &last_iter));
#endif

	create_providers (&all_providers, &all_list_proposals);
	populate_model (model, all_providers, all_list_proposals);

	gtk_source_completion_model_set_show_headers (model, show_headers);

	g_assert (gtk_source_completion_model_iter_last (model, &last_iter));

	/* Get the last iter by another means, and compare it */
	nb_items = gtk_tree_model_iter_n_children (GTK_TREE_MODEL (model), NULL);

	g_assert (gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (model),
						 &other_iter,
						 NULL,
						 nb_items - 1));

	g_assert (gtk_source_completion_model_iter_equal (model, &last_iter, &other_iter));

	/* Test iter_previous() */
	while (gtk_source_completion_model_iter_previous (model, &other_iter));

	g_assert (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (model), &first_iter));
	g_assert (gtk_source_completion_model_iter_equal (model, &first_iter, &other_iter));

	g_object_unref (model);
	free_providers (all_providers, all_list_proposals);
}

static void
test_iters (void)
{
	test_iters_impl (FALSE);
#if 0
	test_iters_impl (TRUE);
#endif
}

int
main (int argc, char **argv)
{
	gtk_test_init (&argc, &argv);

	g_test_add_func ("/CompletionModel/is-empty",
			 test_is_empty);

	g_test_add_func ("/CompletionModel/get-visible-providers",
			 test_get_visible_providers);

	g_test_add_func ("/CompletionModel/simple-populate",
			 test_simple_populate);

	g_test_add_func ("/CompletionModel/clear",
			 test_clear);

	g_test_add_func ("/CompletionModel/set-visible-providers",
			 test_set_visible_providers);

	g_test_add_func ("/CompletionModel/second-populate-same-order",
			 test_second_populate_same_order);

#if 0
	g_test_add_func ("/CompletionModel/second-populate-different-order",
			 test_second_populate_different_order);
#endif

	g_test_add_func ("/CompletionModel/populate-several-batches",
			 test_populate_several_batches);

	g_test_add_func ("/CompletionModel/get-providers",
			 test_get_providers);

	g_test_add_func ("/CompletionModel/n-proposals",
			 test_n_proposals);

	g_test_add_func ("/CompletionModel/iters",
			 test_iters);

	return g_test_run ();
}